#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "decoder.h"
#include "sngp.h"

#define BUFFER_SIZE 4096

// Read a 16-bit little-endian integer.
static uint16_t read_le16(const unsigned char *data)
{
    return (uint16_t)data[0] |
           ((uint16_t)data[1] << 8);
}

// Read a 64-bit little-endian integer.
static uint64_t read_le64(const unsigned char *data)
{
    uint64_t value = 0;

    for (int i = 0; i < 8; i++)
    {
        value |= (uint64_t)data[i] << (i * 8);
    }

    return value;
}

int decoder_extract_payload(
    const char *container_filename,
    const char *output_filename)
{
    FILE *input = NULL;
    FILE *output = NULL;

    unsigned char header[SNGP_HEADER_SIZE];
    unsigned char buffer[BUFFER_SIZE];

    uint64_t original_file_size;
    uint64_t remaining_bytes;

    // Open the SNGP container.
    input = fopen(container_filename, "rb");

    if (input == NULL)
    {
        printf("[DECODER ERROR] Could not open container.\n");
        return -1;
    }

    // Read the complete SNGP header.
    if (fread(header, 1, SNGP_HEADER_SIZE, input) != SNGP_HEADER_SIZE)
    {
        printf("[DECODER ERROR] Invalid or incomplete header.\n");
        fclose(input);
        return -1;
    }

    // Validate magic signature.
    if (memcmp(header, SNGP_MAGIC, 4) != 0)
    {
        printf("[DECODER ERROR] Invalid SNGP signature.\n");
        fclose(input);
        return -1;
    }

    // Validate container version.
    uint16_t version = read_le16(header + 4);

    if (version != SNGP_VERSION)
    {
        printf("[DECODER ERROR] Unsupported SNGP version.\n");
        fclose(input);
        return -1;
    }

    // Validate header size.
    uint16_t header_size = read_le16(header + 6);

    if (header_size != SNGP_HEADER_SIZE)
    {
        printf("[DECODER ERROR] Invalid header size.\n");
        fclose(input);
        return -1;
    }

    // Extract original audio file size.
    original_file_size = read_le64(header + 16);
    remaining_bytes = original_file_size;

    printf("\n====================================\n");
    printf("          SNGP DECODER\n");
    printf("====================================\n");

    printf("Container File : %s\n", container_filename);
    printf("Output File    : %s\n", output_filename);
    printf("Payload Size   : %llu bytes\n",
           (unsigned long long)original_file_size);

    // Open destination file.
    output = fopen(output_filename, "wb");

    if (output == NULL)
    {
        printf("[DECODER ERROR] Could not create output file.\n");
        fclose(input);
        return -1;
    }

    // Extract original audio bytes.
    while (remaining_bytes > 0)
    {
        size_t bytes_to_read =
            remaining_bytes < BUFFER_SIZE
                ? (size_t)remaining_bytes
                : BUFFER_SIZE;

        size_t bytes_read = fread(buffer, 1, bytes_to_read, input);

        if (bytes_read != bytes_to_read)
        {
            printf("[DECODER ERROR] Container payload is incomplete.\n");
            fclose(input);
            fclose(output);
            remove(output_filename);
            return -1;
        }

        size_t bytes_written =
            fwrite(buffer, 1, bytes_read, output);

        if (bytes_written != bytes_read)
        {
            printf("[DECODER ERROR] Failed to write output data.\n");
            fclose(input);
            fclose(output);
            remove(output_filename);
            return -1;
        }

        remaining_bytes -= bytes_read;
    }

    // The container must not contain unexpected trailing data.
    if (fgetc(input) != EOF)
    {
        printf("[DECODER ERROR] Unexpected trailing data.\n");
        fclose(input);
        fclose(output);
        remove(output_filename);
        return -1;
    }

    if (ferror(input))
    {
        printf("[DECODER ERROR] Failed while reading container.\n");
        fclose(input);
        fclose(output);
        remove(output_filename);
        return -1;
    }

    // Check that the output was successfully closed.
    int output_error = fclose(output);
    int input_error = fclose(input);

    if (output_error != 0 || input_error != 0)
    {
        printf("[DECODER ERROR] Failed to finalize files.\n");
        remove(output_filename);
        return -1;
    }

    printf("\n[DECODER] Extraction completed successfully.\n");
    printf("====================================\n");

    return 0;
}