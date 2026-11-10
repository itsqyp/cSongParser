#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <winsock2.h>

#include "tcp_chunk_transfer.h"
#include "tcp_file_transfer.h"
#include "tcp_io.h"
#include "chunker.h"

#define MAX_CHUNKS 1000000
#define FILENAME_BUFFER_SIZE 1024

/* Information extracted from an SNGC header. */
typedef struct
{
    uint16_t version;
    uint16_t header_size;

    uint32_t chunk_index;
    uint32_t total_chunks;
    uint32_t payload_size;

    uint64_t original_file_size;

} SngcChunkInfo;


/* Convert a 32-bit integer into 4 big-endian bytes. */
static void encode_u32_be(
    uint32_t value,
    unsigned char buffer[4]
)
{
    for (int i = 3; i >= 0; i--)
    {
        buffer[i] = (unsigned char)(value & 0xFF);
        value >>= 8;
    }
}


/* Convert 4 big-endian bytes into a 32-bit integer. */
static uint32_t decode_u32_be(
    const unsigned char buffer[4]
)
{
    uint32_t value = 0;

    for (int i = 0; i < 4; i++)
    {
        value = (value << 8) | buffer[i];
    }

    return value;
}


/* Read a little-endian 16-bit integer. */
static uint16_t read_le16(const unsigned char *data)
{
    return (uint16_t)(
        data[0] |
        ((uint16_t)data[1] << 8)
    );
}


/* Read a little-endian 32-bit integer. */
static uint32_t read_le32(const unsigned char *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}


/* Read a little-endian 64-bit integer. */
static uint64_t read_le64(const unsigned char *data)
{
    uint64_t value = 0;

    for (int i = 0; i < 8; i++)
    {
        value |= (uint64_t)data[i] << (i * 8);
    }

    return value;
}


/* Read and validate an SNGC header. */
static int read_sngc_header(
    FILE *file,
    SngcChunkInfo *info
)
{
    unsigned char header[SNGC_HEADER_SIZE];

    if (fread(header, 1, 8, file) != 8)
    {
        printf("[CHUNK TRANSFER ERROR] Could not read SNGC header.\n");
        return -1;
    }

    if (memcmp(header, SNGC_MAGIC, 4) != 0)
    {
        printf("[CHUNK TRANSFER ERROR] Invalid SNGC magic.\n");
        return -1;
    }

    info->version = read_le16(header + 4);
    info->header_size = read_le16(header + 6);

    if (info->version == SNGC_V1_VERSION)
    {
        if (info->header_size != SNGC_V1_HEADER_SIZE)
        {
            printf("[CHUNK TRANSFER ERROR] Invalid SNGC v1 header size.\n");
            return -1;
        }
    }
    else if (info->version == SNGC_VERSION)
    {
        if (info->header_size != SNGC_HEADER_SIZE)
        {
            printf("[CHUNK TRANSFER ERROR] Invalid SNGC v2 header size.\n");
            return -1;
        }
    }
    else
    {
        printf("[CHUNK TRANSFER ERROR] Unsupported SNGC version.\n");
        return -1;
    }

    if (fread(
        header + 8,
        1,
        info->header_size - 8,
        file
    ) != (size_t)(info->header_size - 8))
    {
        printf("[CHUNK TRANSFER ERROR] Incomplete SNGC header.\n");
        return -1;
    }

    info->chunk_index = read_le32(header + 8);
    info->total_chunks = read_le32(header + 12);
    info->payload_size = read_le32(header + 16);

    info->original_file_size = read_le64(header + 24);

    if (info->total_chunks == 0 ||
        info->total_chunks > MAX_CHUNKS ||
        info->chunk_index >= info->total_chunks ||
        info->payload_size == 0 ||
        info->original_file_size == 0)
    {
        printf("[CHUNK TRANSFER ERROR] Invalid SNGC header information.\n");
        return -1;
    }

    return 0;
}


/* Generate a chunk filename. */
static int build_chunk_filename(
    char *filename,
    size_t filename_size,
    const char *prefix,
    uint32_t index
)
{
    int length = snprintf(
        filename,
        filename_size,
        "%s.part%06u.sngc",
        prefix,
        index
    );

    if (length < 0 || (size_t)length >= filename_size)
    {
        printf("[CHUNK TRANSFER ERROR] Filename is too long.\n");
        return -1;
    }

    return 0;
}


/* Validate the physical size of a chunk file. */
static int validate_chunk_file_size(
    FILE *file,
    const SngcChunkInfo *info
)
{
    if (_fseeki64(file, 0, SEEK_END) != 0)
    {
        printf("[CHUNK TRANSFER ERROR] Could not seek to file end.\n");
        return -1;
    }

    __int64 file_size = _ftelli64(file);

    if (file_size < 0)
    {
        printf("[CHUNK TRANSFER ERROR] Could not determine file size.\n");
        return -1;
    }

    uint64_t expected_size =
        (uint64_t)info->header_size + info->payload_size;

    if ((uint64_t)file_size != expected_size)
    {
        printf("[CHUNK TRANSFER ERROR] Chunk file size mismatch.\n");
        return -1;
    }

    return 0;
}


/*
 * Discover the total chunk count from the first chunk
 * and validate all expected chunk files.
 */
static int discover_chunks(
    const char *input_prefix,
    uint32_t *total_chunks
)
{
    char filename[FILENAME_BUFFER_SIZE];

    FILE *file = NULL;

    SngcChunkInfo reference_info;
    SngcChunkInfo current_info;

    /* Locate the first chunk. */
    if (build_chunk_filename(
        filename,
        sizeof(filename),
        input_prefix,
        0
    ) != 0)
    {
        return -1;
    }

    file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("[CHUNK TRANSFER ERROR] First chunk not found: %s\n",
               filename);
        return -1;
    }

    /* Read the first chunk header. */
    if (read_sngc_header(file, &reference_info) != 0)
    {
        fclose(file);
        return -1;
    }

    fclose(file);
    file = NULL;

    if (reference_info.chunk_index != 0)
    {
        printf("[CHUNK TRANSFER ERROR] First chunk has invalid index.\n");
        return -1;
    }

    /* Calculate the expected number of chunks. */
    uint64_t expected_total =
        reference_info.original_file_size /
        reference_info.payload_size;

    if (reference_info.original_file_size %
        reference_info.payload_size != 0)
    {
        expected_total++;
    }

    if (expected_total != reference_info.total_chunks)
    {
        printf("[CHUNK TRANSFER ERROR] Invalid total chunk count.\n");
        return -1;
    }

    printf("[CHUNK TRANSFER] Discovered %u chunks.\n",
           reference_info.total_chunks);

    /*
     * Validate every expected chunk before
     * starting the network transfer.
     */
    for (uint32_t i = 0; i < reference_info.total_chunks; i++)
    {
        if (build_chunk_filename(
            filename,
            sizeof(filename),
            input_prefix,
            i
        ) != 0)
        {
            return -1;
        }

        file = fopen(filename, "rb");

        if (file == NULL)
        {
            printf("[CHUNK TRANSFER ERROR] Missing chunk: %s\n",
                   filename);
            return -1;
        }

        if (read_sngc_header(file, &current_info) != 0)
        {
            fclose(file);
            return -1;
        }

        if (current_info.chunk_index != i ||
            current_info.version != reference_info.version ||
            current_info.header_size != reference_info.header_size ||
            current_info.total_chunks != reference_info.total_chunks ||
            current_info.original_file_size != reference_info.original_file_size)
        {
            printf("[CHUNK TRANSFER ERROR] Inconsistent chunk metadata: %s\n",
                   filename);

            fclose(file);
            return -1;
        }

        /* Calculate the expected payload size. */
        uint64_t bytes_before_chunk =
            (uint64_t)i * reference_info.payload_size;

        uint64_t remaining_bytes =
            reference_info.original_file_size - bytes_before_chunk;

        uint32_t expected_payload_size =
            remaining_bytes < reference_info.payload_size
                ? (uint32_t)remaining_bytes
                : reference_info.payload_size;

        if (current_info.payload_size != expected_payload_size)
        {
            printf("[CHUNK TRANSFER ERROR] Invalid payload size: %s\n",
                   filename);

            fclose(file);
            return -1;
        }

        if (validate_chunk_file_size(file, &current_info) != 0)
        {
            fclose(file);
            return -1;
        }

        fclose(file);
        file = NULL;
    }

    printf("[CHUNK TRANSFER] All %u chunks validated.\n",
           reference_info.total_chunks);

    *total_chunks = reference_info.total_chunks;

    return 0;
}


/* Send multiple SNGC chunk files. */
int tcp_send_chunks(
    SOCKET socket,
    const char *input_prefix
)
{
    if (input_prefix == NULL || input_prefix[0] == '\0')
    {
        printf("[CHUNK TRANSFER ERROR] Invalid input prefix.\n");
        return -1;
    }

    uint32_t total_chunks = 0;

    /* Automatically discover and validate chunks. */
    if (discover_chunks(input_prefix, &total_chunks) != 0)
    {
        return -1;
    }

    /* Send total chunk count. */
    unsigned char count_header[4];

    encode_u32_be(total_chunks, count_header);

    if (tcp_send_all(socket, count_header, 4) != 0)
    {
        printf("[CHUNK TRANSFER ERROR] Failed to send chunk count.\n");
        return -1;
    }

    printf("[CHUNK TRANSFER] Sending %u chunks.\n", total_chunks);

    /* Send chunks sequentially. */
    for (uint32_t i = 0; i < total_chunks; i++)
    {
        char filename[FILENAME_BUFFER_SIZE];

        if (build_chunk_filename(
            filename,
            sizeof(filename),
            input_prefix,
            i
        ) != 0)
        {
            return -1;
        }

        printf("[CHUNK TRANSFER] Sending chunk %u/%u.\n",
               i + 1,
               total_chunks);

        if (tcp_send_file(socket, filename) != 0)
        {
            printf("[CHUNK TRANSFER ERROR] Failed to send chunk %u.\n", i);
            return -1;
        }
    }

    printf("[CHUNK TRANSFER] All chunks sent successfully.\n");

    return 0;
}


/* Receive multiple SNGC chunk files. */
int tcp_receive_chunks(
    SOCKET socket,
    const char *output_prefix
)
{
    if (output_prefix == NULL || output_prefix[0] == '\0')
    {
        printf("[CHUNK TRANSFER ERROR] Invalid output prefix.\n");
        return -1;
    }

    /* Receive total chunk count. */
    unsigned char count_header[4];

    if (tcp_receive_all(socket, count_header, 4) != 0)
    {
        printf("[CHUNK TRANSFER ERROR] Failed to receive chunk count.\n");
        return -1;
    }

    uint32_t total_chunks = decode_u32_be(count_header);

    if (total_chunks == 0 || total_chunks > MAX_CHUNKS)
    {
        printf("[CHUNK TRANSFER ERROR] Invalid chunk count: %u\n",
               total_chunks);
        return -1;
    }

    printf("[CHUNK TRANSFER] Expecting %u chunks.\n", total_chunks);

    /* Receive chunks sequentially. */
    for (uint32_t i = 0; i < total_chunks; i++)
    {
        char filename[FILENAME_BUFFER_SIZE];

        if (build_chunk_filename(
            filename,
            sizeof(filename),
            output_prefix,
            i
        ) != 0)
        {
            return -1;
        }

        printf("[CHUNK TRANSFER] Receiving chunk %u/%u.\n",
               i + 1,
               total_chunks);

        if (tcp_receive_file(socket, filename) != 0)
        {
            printf("[CHUNK TRANSFER ERROR] Failed to receive chunk %u.\n", i);
            return -1;
        }
    }

    printf("[CHUNK TRANSFER] All chunks received successfully.\n");

    return 0;
}