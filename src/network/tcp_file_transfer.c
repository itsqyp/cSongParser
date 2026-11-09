#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>

#include "tcp_file_transfer.h"
#include "tcp_io.h"


#define FILE_BUFFER_SIZE 4096
#define MAX_FILE_SIZE (1024ULL * 1024ULL * 1024ULL)


int tcp_send_file(
    SOCKET socket,
    const char *filename
)
{
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("[TRANSFER ERROR] Could not open file: %s\n", filename);

        return -1;
    }

    // Determine file size.
    if (_fseeki64(file, 0, SEEK_END) != 0)
    {
        printf("[TRANSFER ERROR] Could not determine file size.\n");

        fclose(file);

        return -1;
    }

    __int64 file_size = _ftelli64(file);

    if (file_size <= 0 || (uint64_t)file_size > MAX_FILE_SIZE)
    {
        printf("[TRANSFER ERROR] Invalid or unsupported file size.\n");

        fclose(file);

        return -1;
    }

    rewind(file);

    // Send file size.
    uint64_t network_file_size = htonll((uint64_t)file_size);

    if (tcp_send_all(
            socket,
            &network_file_size,
            sizeof(network_file_size)
        ) != 0)
    {
        fclose(file);

        return -1;
    }

    printf("[TRANSFER] Sending file: %s\n", filename);
    printf("[TRANSFER] File size: %llu bytes\n",
           (unsigned long long)file_size);

    // Transfer file contents.
    unsigned char buffer[FILE_BUFFER_SIZE];

    uint64_t total_sent = 0;

    while (total_sent < (uint64_t)file_size)
    {
        size_t bytes_to_read = FILE_BUFFER_SIZE;

        uint64_t remaining = (uint64_t)file_size - total_sent;

        if (remaining < bytes_to_read)
        {
            bytes_to_read = (size_t)remaining;
        }

        size_t bytes_read = fread(
            buffer,
            1,
            bytes_to_read,
            file
        );

        if (bytes_read != bytes_to_read)
        {
            printf("[TRANSFER ERROR] Failed to read file completely.\n");

            fclose(file);

            return -1;
        }

        if (tcp_send_all(
                socket,
                buffer,
                (int)bytes_read
            ) != 0)
        {
            fclose(file);

            return -1;
        }

        total_sent += bytes_read;
    }

    fclose(file);

    printf("[TRANSFER] Successfully sent %llu bytes.\n",
           (unsigned long long)total_sent);

    return 0;
}


int tcp_receive_file(
    SOCKET socket,
    const char *output_filename
)
{
    // Receive file size.
    uint64_t network_file_size;

    if (tcp_receive_all(
            socket,
            &network_file_size,
            sizeof(network_file_size)
        ) != 0)
    {
        return -1;
    }

    uint64_t file_size = ntohll(network_file_size);

    // Validate file size.
    if (file_size == 0 || file_size > MAX_FILE_SIZE)
    {
        printf("[TRANSFER ERROR] Invalid or unsupported file size.\n");

        return -1;
    }

    // Create destination file.
    FILE *file = fopen(output_filename, "wb");

    if (file == NULL)
    {
        printf("[TRANSFER ERROR] Could not create output file.\n");

        return -1;
    }

    printf("[TRANSFER] Receiving file: %s\n", output_filename);
    printf("[TRANSFER] Expected size: %llu bytes\n",
           (unsigned long long)file_size);

    // Receive file contents.
    unsigned char buffer[FILE_BUFFER_SIZE];

    uint64_t total_received = 0;

    while (total_received < file_size)
    {
        uint64_t remaining = file_size - total_received;

        int bytes_to_receive = FILE_BUFFER_SIZE;

        if (remaining < (uint64_t)bytes_to_receive)
        {
            bytes_to_receive = (int)remaining;
        }

        if (tcp_receive_all(
                socket,
                buffer,
                bytes_to_receive
            ) != 0)
        {
            fclose(file);
            remove(output_filename);

            return -1;
        }

        size_t bytes_written = fwrite(
            buffer,
            1,
            (size_t)bytes_to_receive,
            file
        );

        if (bytes_written != (size_t)bytes_to_receive)
        {
            printf("[TRANSFER ERROR] Failed to write file completely.\n");

            fclose(file);
            remove(output_filename);

            return -1;
        }

        total_received += bytes_written;
    }

    if (fclose(file) != 0)
    {
        printf("[TRANSFER ERROR] Failed to close output file.\n");

        remove(output_filename);

        return -1;
    }

    printf("[TRANSFER] Successfully received %llu bytes.\n",
           (unsigned long long)total_received);

    return 0;
}