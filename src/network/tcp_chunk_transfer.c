#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>

#include "tcp_chunk_transfer.h"
#include "tcp_file_transfer.h"
#include "tcp_io.h"

#define MAX_CHUNKS 1000000
#define FILENAME_BUFFER_SIZE 1024

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

/* Send multiple SNGC chunk files. */
int tcp_send_chunks(
    SOCKET socket,
    const char *input_prefix,
    uint32_t total_chunks
)
{
    if (input_prefix == NULL ||
        total_chunks == 0 ||
        total_chunks > MAX_CHUNKS)
    {
        printf("[CHUNK TRANSFER ERROR] Invalid transfer parameters.\n");
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

        int length = snprintf(
            filename,
            sizeof(filename),
            "%s.part%06u.sngc",
            input_prefix,
            i
        );

        if (length < 0 || (size_t)length >= sizeof(filename))
        {
            printf("[CHUNK TRANSFER ERROR] Filename is too long.\n");
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
    if (output_prefix == NULL)
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

        int length = snprintf(
            filename,
            sizeof(filename),
            "%s.part%06u.sngc",
            output_prefix,
            i
        );

        if (length < 0 || (size_t)length >= sizeof(filename))
        {
            printf("[CHUNK TRANSFER ERROR] Filename is too long.\n");
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