#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "chunker.h"
#include "checksum.h"

#define BUFFER_SIZE 4096

// Write a 16-bit little-endian integer.
static void write_le16(unsigned char *data, uint16_t value)
{
    data[0] = (unsigned char)(value);
    data[1] = (unsigned char)(value >> 8);
}

// Write a 32-bit little-endian integer.
static void write_le32(unsigned char *data, uint32_t value)
{
    for (int i = 0; i < 4; i++)
    {
        data[i] = (unsigned char)(value >> (i * 8));
    }
}

// Write a 64-bit little-endian integer.
static void write_le64(unsigned char *data, uint64_t value)
{
    for (int i = 0; i < 8; i++)
    {
        data[i] = (unsigned char)(value >> (i * 8));
    }
}

// Build the SNGC chunk header.
static void build_chunk_header(
    unsigned char *header,
    uint32_t chunk_index,
    uint32_t total_chunks,
    uint32_t payload_size,
    uint64_t original_file_size,
    const unsigned char checksum[SHA256_HASH_SIZE]
)
{
    memset(header, 0, SNGC_HEADER_SIZE);

    memcpy(header, SNGC_MAGIC, 4);

    write_le16(header + 4, SNGC_VERSION);
    write_le16(header + 6, SNGC_HEADER_SIZE);

    write_le32(header + 8, chunk_index);
    write_le32(header + 12, total_chunks);
    write_le32(header + 16, payload_size);

    write_le64(header + 24, original_file_size);

    // Store the payload SHA-256 checksum.
    memcpy(
        header + SNGC_CHECKSUM_OFFSET,
        checksum,
        SHA256_HASH_SIZE
    );
}

int chunker_create_chunks(
    const char *input_filename,
    const char *output_prefix,
    uint32_t chunk_size)
{
    FILE *input = NULL;
    FILE *output = NULL;

    unsigned char header[SNGC_HEADER_SIZE];
    unsigned char buffer[BUFFER_SIZE];
    unsigned char chunk_checksum[SHA256_HASH_SIZE];
    uint64_t original_file_size;
    uint64_t remaining_bytes;

    uint32_t total_chunks;
    uint32_t chunk_index = 0;

    char output_filename[512];

    // Validate arguments.
    if (input_filename == NULL ||
        output_prefix == NULL ||
        chunk_size == 0)
    {
        printf("[CHUNKER ERROR] Invalid arguments.\n");
        return -1;
    }

    // Open input file.
    input = fopen(input_filename, "rb");

    if (input == NULL)
    {
        printf("[CHUNKER ERROR] Could not open input file.\n");
        return -1;
    }

    // Determine input file size.
    if (fseek(input, 0, SEEK_END) != 0)
    {
        printf("[CHUNKER ERROR] Could not determine file size.\n");
        fclose(input);
        return -1;
    }

    long file_size = ftell(input);

    if (file_size < 0)
    {
        printf("[CHUNKER ERROR] Invalid file size.\n");
        fclose(input);
        return -1;
    }

    original_file_size = (uint64_t)file_size;

    if (fseek(input, 0, SEEK_SET) != 0)
    {
        printf("[CHUNKER ERROR] Could not rewind input file.\n");
        fclose(input);
        return -1;
    }

    // Calculate total number of chunks.
    total_chunks = (uint32_t)(
        original_file_size / chunk_size +
        (original_file_size % chunk_size != 0)
    );

    remaining_bytes = original_file_size;

    printf("\n====================================\n");
    printf("          SNGP CHUNKER\n");
    printf("====================================\n");

    printf("Input File      : %s\n", input_filename);
    printf("Original Size   : %llu bytes\n",
           (unsigned long long)original_file_size);

    printf("Chunk Size      : %u bytes\n", chunk_size);
    printf("Total Chunks    : %u\n", total_chunks);

    // Create individual chunks.
    while (remaining_bytes > 0)
    {
        uint32_t current_payload_size =
            remaining_bytes < chunk_size
                ? (uint32_t)remaining_bytes
                : chunk_size;

        // Generate chunk filename.
        snprintf(
            output_filename,
            sizeof(output_filename),
            "%s.part%06u.sngc",
            output_prefix,
            chunk_index
        );

        // Open chunk output file.
        output = fopen(output_filename, "wb");

        if (output == NULL)
        {
            printf("[CHUNKER ERROR] Could not create chunk file.\n");
            goto cleanup;
        }

        // Build chunk header.
      // Calculate SHA-256 of the current chunk payload.
      if (checksum_sha256_file_region(
        input_filename,
        (uint64_t)chunk_index * chunk_size,
        current_payload_size,
        chunk_checksum
        ) != 0)
        {
        printf("[CHUNKER ERROR] Could not calculate chunk checksum.\n");
        goto cleanup;
         }

         // Build SNGC v2 header.
        build_chunk_header(
        header,
       chunk_index,
       total_chunks,
       current_payload_size,
       original_file_size,
       chunk_checksum
        );

        // Write chunk header.
        if (fwrite(header, 1, SNGC_HEADER_SIZE, output)
            != SNGC_HEADER_SIZE)
        {
            printf("[CHUNKER ERROR] Failed to write chunk header.\n");
            goto cleanup;
        }

        uint32_t chunk_remaining = current_payload_size;

        // Copy payload into current chunk.
        while (chunk_remaining > 0)
        {
            size_t bytes_to_read =
                chunk_remaining < BUFFER_SIZE
                    ? chunk_remaining
                    : BUFFER_SIZE;

            size_t bytes_read =
                fread(buffer, 1, bytes_to_read, input);

            if (bytes_read != bytes_to_read)
            {
                printf("[CHUNKER ERROR] Failed to read input data.\n");
                goto cleanup;
            }

            if (fwrite(buffer, 1, bytes_read, output)
                != bytes_read)
            {
                printf("[CHUNKER ERROR] Failed to write chunk data.\n");
                goto cleanup;
            }

            chunk_remaining -= (uint32_t)bytes_read;
        }

        // Close current chunk.
        if (fclose(output) != 0)
        {
            output = NULL;

            printf("[CHUNKER ERROR] Failed to finalize chunk.\n");
            goto cleanup;
        }

        output = NULL;

        printf("[CHUNKER] Created: %s (%u bytes)\n",
               output_filename,
               current_payload_size);

        remaining_bytes -= current_payload_size;
        chunk_index++;
    }

    fclose(input);

    printf("\n[CHUNKER] Chunking completed successfully.\n");
    printf("====================================\n");

    return 0;

cleanup:

    if (output != NULL)
    {
        fclose(output);
        remove(output_filename);
    }

    if (input != NULL)
    {
        fclose(input);
    }

    return -1;
}