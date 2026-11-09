#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "reassembler.h"
#include "chunker.h"
#include "checksum.h"


#define BUFFER_SIZE 4096
#define CHUNK_FILENAME_SIZE 512


static uint16_t read_le16(const unsigned char *data)
{
    return (uint16_t)(
        data[0] |
        ((uint16_t)data[1] << 8)
    );
}

static uint32_t read_le32(const unsigned char *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint64_t read_le64(const unsigned char *data)
{
    uint64_t value = 0;

    for (int i = 0; i < 8; i++)
    {
        value |= (uint64_t)data[i] << (i * 8);
    }

    return value;
}

typedef struct
{
    uint16_t version;
    uint16_t header_size;

    uint32_t chunk_index;
    uint32_t total_chunks;
    uint32_t payload_size;

    uint64_t original_file_size;

    unsigned char checksum[SHA256_HASH_SIZE];

} SngcChunkInfo;


static int read_chunk_header(
    FILE *chunk_file,
    SngcChunkInfo *info
)
{
    unsigned char header[SNGC_HEADER_SIZE];

    // Read the first 8 bytes.
    if (fread(header, 1, 8, chunk_file) != 8)
    {
        printf("[REASSEMBLER ERROR] Could not read chunk header.\n");
        return -1;
    }

    // Validate magic bytes.
    if (memcmp(header, SNGC_MAGIC, 4) != 0)
    {
        printf("[REASSEMBLER ERROR] Invalid chunk magic.\n");
        return -1;
    }

    // Extract version and header size.
    info->version = read_le16(header + 4);
    info->header_size = read_le16(header + 6);

    // Validate supported versions and their header sizes.
    if (info->version == SNGC_V1_VERSION)
    {
        if (info->header_size != SNGC_V1_HEADER_SIZE)
        {
            printf("[REASSEMBLER ERROR] Invalid SNGC v1 header size.\n");
            return -1;
        }
    }
    else if (info->version == SNGC_VERSION)
    {
        if (info->header_size != SNGC_HEADER_SIZE)
        {
            printf("[REASSEMBLER ERROR] Invalid SNGC v2 header size.\n");
            return -1;
        }
    }
    else
    {
        printf("[REASSEMBLER ERROR] Unsupported chunk version.\n");
        return -1;
    }

    // Read the remaining header bytes.
    if (fread(
            header + 8,
            1,
            info->header_size - 8,
            chunk_file
        ) != (size_t)(info->header_size - 8))
    {
        printf("[REASSEMBLER ERROR] Incomplete chunk header.\n");
        return -1;
    }

    // Extract common header information.
    info->chunk_index = read_le32(header + 8);
    info->total_chunks = read_le32(header + 12);
    info->payload_size = read_le32(header + 16);

    info->original_file_size = read_le64(header + 24);

    // Extract checksum for SNGC v2.
    memset(info->checksum, 0, SHA256_HASH_SIZE);

    if (info->version == SNGC_VERSION)
    {
        memcpy(
            info->checksum,
            header + SNGC_CHECKSUM_OFFSET,
            SHA256_HASH_SIZE
        );
    }

    // Validate extracted information.
    if (info->total_chunks == 0 ||
        info->chunk_index >= info->total_chunks ||
        info->payload_size == 0 ||
        info->original_file_size == 0)
    {
        printf("[REASSEMBLER ERROR] Invalid chunk header information.\n");
        return -1;
    }

    return 0;
}


static int validate_chunk_file(
    FILE *chunk_file,
    const SngcChunkInfo *info
)
{
    // Move to the end of the chunk file.
    if (_fseeki64(chunk_file, 0, SEEK_END) != 0)
    {
        printf("[REASSEMBLER ERROR] Could not determine chunk file size.\n");
        return -1;
    }

    __int64 file_size = _ftelli64(chunk_file);

    if (file_size < 0)
    {
        printf("[REASSEMBLER ERROR] Invalid chunk file size.\n");
        return -1;
    }

    // Expected size = header + payload.
    uint64_t expected_size =
        (uint64_t)info->header_size + info->payload_size;

    if ((uint64_t)file_size != expected_size)
    {
        printf("[REASSEMBLER ERROR] Chunk file size mismatch.\n");

        printf("Expected Size : %llu bytes\n",
               (unsigned long long)expected_size);

        printf("Actual Size   : %lld bytes\n",
               (long long)file_size);

        return -1;
    }

    // Return to the beginning of the payload.
    if (_fseeki64(chunk_file, info->header_size, SEEK_SET) != 0)
    {
        printf("[REASSEMBLER ERROR] Could not seek to chunk payload.\n");
        return -1;
    }

    return 0;
}

// Verify the SHA-256 checksum of a chunk payload.
static int verify_chunk_checksum(
    const char *chunk_filename,
    const SngcChunkInfo *info
)
{
    // SNGC v1 does not contain chunk checksums.
    if (info->version == SNGC_V1_VERSION)
    {
        return 0;
    }

    unsigned char calculated_checksum[SHA256_HASH_SIZE];

    // Calculate SHA-256 of the chunk payload.
    if (checksum_sha256_file_region(
            chunk_filename,
            info->header_size,
            info->payload_size,
            calculated_checksum
        ) != 0)
    {
        printf("[REASSEMBLER ERROR] Could not calculate chunk checksum.\n");
        return -1;
    }

    // Compare calculated checksum with stored checksum.
    if (memcmp(
            calculated_checksum,
            info->checksum,
            SHA256_HASH_SIZE
        ) != 0)
    {
        printf("[REASSEMBLER ERROR] Chunk checksum mismatch!\n");

        printf("[REASSEMBLER ERROR] Chunk may be corrupted: %s\n",
               chunk_filename);

        return -1;
    }

    printf("[REASSEMBLER] Chunk checksum verified.\n");

    return 0;
}


int reassembler_combine_chunks(
    const char *input_prefix,
    const char *output_filename
)
{
    FILE *chunk_file = NULL;
    FILE *output = NULL;
    int output_created = 0;

    unsigned char buffer[BUFFER_SIZE];

    char chunk_filename[CHUNK_FILENAME_SIZE];

    SngcChunkInfo reference_info;
    SngcChunkInfo current_info;

    uint64_t total_payload_written = 0;

    int result = -1;

    // Validate arguments.
    if (input_prefix == NULL ||
        output_filename == NULL ||
        input_prefix[0] == '\0' ||
        output_filename[0] == '\0')
    {
        printf("[REASSEMBLER ERROR] Invalid arguments.\n");
        return -1;
    }

    printf("\n====================================\n");
    printf("         SNGC REASSEMBLER\n");
    printf("====================================\n");

    printf("Input Prefix    : %s\n", input_prefix);
    printf("Output File     : %s\n", output_filename);

    // Read the first chunk to obtain reference information.
   int filename_length = snprintf(
    chunk_filename,
    sizeof(chunk_filename),
    "%s.part%06u.sngc",
    input_prefix,
    0
);

if (filename_length < 0 ||
    (size_t)filename_length >= sizeof(chunk_filename))
{
    printf("[REASSEMBLER ERROR] Chunk filename is too long.\n");
    return -1;
}

    chunk_file = fopen(chunk_filename, "rb");

    if (chunk_file == NULL)
    {
        printf("[REASSEMBLER ERROR] Could not open first chunk.\n");
        goto cleanup;
    }

    if (read_chunk_header(chunk_file, &reference_info) != 0)
    {
        goto cleanup;
    }

    if (reference_info.chunk_index != 0)
    {
        printf("[REASSEMBLER ERROR] First chunk has an invalid index.\n");
        goto cleanup;
    }

    if (validate_chunk_file(chunk_file, &reference_info) != 0)
    {
        goto cleanup;
    }

    fclose(chunk_file);
    chunk_file = NULL;

    printf("\nOriginal Size   : %llu bytes\n",
           (unsigned long long)reference_info.original_file_size);

    printf("Total Chunks    : %u\n",
           reference_info.total_chunks);

    // Create the reconstructed output file.
  output = fopen(output_filename, "wbx");

if (output == NULL)
{
    printf("[REASSEMBLER ERROR] Could not create output file.\n");
    goto cleanup;
}

output_created = 1;

    // Process every chunk in order.
    for (uint32_t i = 0; i < reference_info.total_chunks; i++)
    {
        // Generate the current chunk filename.
        int filename_length = snprintf(
            chunk_filename,
            sizeof(chunk_filename),
            "%s.part%06u.sngc",
            input_prefix,
            i
        );

       if (filename_length < 0 ||
    (size_t)filename_length >= sizeof(chunk_filename))
        {
            printf("[REASSEMBLER ERROR] Chunk filename is too long.\n");
            goto cleanup;
        }

        // Open the current chunk.
        chunk_file = fopen(chunk_filename, "rb");

        if (chunk_file == NULL)
        {
            printf("[REASSEMBLER ERROR] Missing chunk: %s\n",
                   chunk_filename);
            goto cleanup;
        }

        // Read and validate its header.
        if (read_chunk_header(chunk_file, &current_info) != 0)
        {
            goto cleanup;
        }

        // Validate chunk index.
        if (current_info.chunk_index != i)
        {
            printf("[REASSEMBLER ERROR] Incorrect chunk index.\n");
            goto cleanup;
        }

       // Ensure all chunks belong to the same container.
if (current_info.version != reference_info.version ||
    current_info.header_size != reference_info.header_size ||
    current_info.total_chunks != reference_info.total_chunks ||
    current_info.original_file_size != reference_info.original_file_size)
{
    printf("[REASSEMBLER ERROR] Inconsistent chunk metadata.\n");
    goto cleanup;
}

      // Validate actual chunk file size.
      if (validate_chunk_file(chunk_file, &current_info) != 0)
     {
      goto cleanup;
    }

    // Verify the chunk's SHA-256 checksum.
    if (verify_chunk_checksum(chunk_filename, &current_info) != 0)
     {
     goto cleanup;
     }

     // Copy the chunk payload into the output file.
     uint32_t remaining_bytes = current_info.payload_size;

        while (remaining_bytes > 0)
        {
            size_t bytes_to_read =
                remaining_bytes < BUFFER_SIZE
                    ? remaining_bytes
                    : BUFFER_SIZE;

            size_t bytes_read = fread(
                buffer,
                1,
                bytes_to_read,
                chunk_file
            );

            if (bytes_read != bytes_to_read)
            {
                printf("[REASSEMBLER ERROR] Failed to read chunk payload.\n");
                goto cleanup;
            }

            if (fwrite(buffer, 1, bytes_read, output) != bytes_read)
            {
                printf("[REASSEMBLER ERROR] Failed to write output data.\n");
                goto cleanup;
            }

            remaining_bytes -= (uint32_t)bytes_read;
            total_payload_written += bytes_read;
        }

        fclose(chunk_file);
        chunk_file = NULL;

        printf("[REASSEMBLER] Processed chunk %u/%u\n",
               i + 1,
               reference_info.total_chunks);
    }

    // Verify the reconstructed file size.
    if (total_payload_written != reference_info.original_file_size)
    {
        printf("[REASSEMBLER ERROR] Reconstructed size mismatch.\n");

        printf("Expected Size : %llu bytes\n",
               (unsigned long long)reference_info.original_file_size);

        printf("Actual Size   : %llu bytes\n",
               (unsigned long long)total_payload_written);

        goto cleanup;
    }

    // Finalize output.
    if (fclose(output) != 0)
    {
        output = NULL;

        printf("[REASSEMBLER ERROR] Failed to finalize output file.\n");
        goto cleanup;
    }

    output = NULL;

    printf("\n[REASSEMBLER] Reassembly completed successfully.\n");
    printf("Output Size    : %llu bytes\n",
           (unsigned long long)total_payload_written);

    printf("====================================\n");

    result = 0;

cleanup:

    if (chunk_file != NULL)
    {
        fclose(chunk_file);
    }

    if (output != NULL)
    {
        fclose(output);
    }

    // Remove incomplete output only if this operation created it.
    if (result != 0 && output_created)
    {
    remove(output_filename);
    }

    return result;
}