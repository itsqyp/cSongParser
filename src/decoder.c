#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "decoder.h"
#include "sngp.h"
#include "checksum.h"

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

    unsigned char calculated_checksum[SHA256_HASH_SIZE];

    uint64_t original_file_size;
    uint64_t remaining_bytes;

    uint16_t version;
    uint16_t header_size;

    int output_created = 0;
    int result = -1;

    // Validate arguments.
    if (container_filename == NULL || output_filename == NULL)
    {
        printf("[DECODER ERROR] Invalid file path.\n");
        return -1;
    }

    if (strcmp(container_filename, output_filename) == 0)
    {
        printf("[DECODER ERROR] Input and output paths must be different.\n");
        return -1;
    }

    // Open the SNGP container.
    input = fopen(container_filename, "rb");

    if (input == NULL)
    {
        printf("[DECODER ERROR] Could not open container.\n");
        return -1;
    }

    // Read the first 8 bytes of the header.
    if (fread(header, 1, 8, input) != 8)
    {
        printf("[DECODER ERROR] Incomplete SNGP header.\n");
        goto cleanup;
    }

    // Validate magic signature.
    if (memcmp(header, SNGP_MAGIC, 4) != 0)
    {
        printf("[DECODER ERROR] Invalid SNGP signature.\n");
        goto cleanup;
    }

    // Read version and header size.
    version = read_le16(header + 4);
    header_size = read_le16(header + 6);

    // Validate supported versions and their header sizes.
    if (version == SNGP_V1_VERSION)
    {
        if (header_size != SNGP_V1_HEADER_SIZE)
        {
            printf("[DECODER ERROR] Invalid SNGP v1 header size.\n");
            goto cleanup;
        }
    }
    else if (version == SNGP_VERSION)
    {
        if (header_size != SNGP_HEADER_SIZE)
        {
            printf("[DECODER ERROR] Invalid SNGP v2 header size.\n");
            goto cleanup;
        }
    }
    else
    {
        printf("[DECODER ERROR] Unsupported SNGP version.\n");
        goto cleanup;
    }

    // Read the remaining header bytes.
    size_t remaining_header_size = (size_t)header_size - 8;

if (fread(header + 8, 1, remaining_header_size, input)
    != remaining_header_size)
{
    printf("[DECODER ERROR] Incomplete SNGP header.\n");
    goto cleanup;
}

    // Extract original audio file size.
    original_file_size = read_le64(header + 16);
    remaining_bytes = original_file_size;

    printf("\n====================================\n");
    printf("          SNGP DECODER\n");
    printf("====================================\n");

    printf("Container File : %s\n", container_filename);
    printf("Output File    : %s\n", output_filename);
    printf("Version        : %u\n", version);
    printf("Header Size    : %u bytes\n", header_size);
    printf("Payload Size   : %llu bytes\n",
           (unsigned long long)original_file_size);

    // Open destination file.
    output = fopen(output_filename, "wbx");

    if (output == NULL)
    {
        printf("[DECODER ERROR] Could not create output file.\n");
        goto cleanup;
    }

    output_created = 1;

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
            goto cleanup;
        }

        size_t bytes_written =
            fwrite(buffer, 1, bytes_read, output);

        if (bytes_written != bytes_read)
        {
            printf("[DECODER ERROR] Failed to write output data.\n");
            goto cleanup;
        }

        remaining_bytes -= bytes_read;
    }

    // Check for unexpected trailing data.
    if (fgetc(input) != EOF)
    {
        printf("[DECODER ERROR] Unexpected trailing data.\n");
        goto cleanup;
    }

    if (ferror(input))
    {
        printf("[DECODER ERROR] Failed while reading container.\n");
        goto cleanup;
    }

    // Finalize output file.
    if (fclose(output) != 0)
    {
        output = NULL;

        printf("[DECODER ERROR] Failed to finalize output file.\n");
        goto cleanup;
    }

    output = NULL;

    // Close container file.
    if (fclose(input) != 0)
    {
        input = NULL;

        printf("[DECODER ERROR] Failed to close container file.\n");
        goto cleanup;
    }

    input = NULL;

    // Verify payload integrity for SNGP v2.
    if (version == SNGP_VERSION)
    {
        printf("\n[DECODER] Verifying SHA-256 checksum...\n");

        if (checksum_sha256_file(
                output_filename,
                calculated_checksum) != 0)
        {
            printf("[DECODER ERROR] Failed to calculate output checksum.\n");
            goto cleanup;
        }

        if (memcmp(
                calculated_checksum,
                header + SNGP_CHECKSUM_OFFSET,
                SHA256_HASH_SIZE) != 0)
        {
            printf("[DECODER ERROR] SHA-256 verification failed!\n");
            printf("[DECODER ERROR] Extracted file may be corrupted.\n");
            goto cleanup;
        }

        printf("[DECODER] SHA-256 verification passed.\n");
    }
    else
    {
        printf("\n[DECODER] SNGP v1 has no embedded checksum.\n");
    }

    printf("\n[DECODER] Extraction completed successfully.\n");
    printf("====================================\n");

    result = 0;

cleanup:

    if (input != NULL)
    {
        fclose(input);
    }

    if (output != NULL)
    {
        fclose(output);
    }

    // Remove incomplete or invalid output files.
    if (result != 0 && output_created)
    {
        remove(output_filename);
    }

    return result;
}