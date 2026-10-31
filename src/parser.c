#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "parser.h"

#define BUFFER_SIZE 4096

int parser_process_file(const char *filename)
{
    // Step 1: Open the file in binary read mode

    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        perror("[ERROR] Could not open file");
        return 1;
    }

    printf("\n====================================\n");
    printf("          BINARY FILE READER        \n");
    printf("====================================\n\n");

    printf("File: %s\n", filename);

    // Step 2: Create a fixed-size buffer

    unsigned char buffer[BUFFER_SIZE];

    // Step 3: Initialize counters

    uint64_t total_bytes = 0;
    uint64_t total_chunks = 0;

    size_t bytes_read;

    printf("\nReading binary data...\n\n");

    // Step 4: Read the file chunk by chunk

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        total_bytes += bytes_read;
        total_chunks++;

        /*
         * Binary data is now available inside buffer.
         *
         * Later, I'll implement:
         * - Audio format detection
         * - Metadata extraction
         * - Binary encoding
         */
    }

    // Step 5: Check for reading errors

    if (ferror(file))
    {
        perror("[ERROR] Failed while reading file");
        fclose(file);
        return 1;
    }

    // Step 6: Close the file

    fclose(file);

    // Step 7: Display statistics

    double file_size_mb = (double)total_bytes / (1024.0 * 1024.0);

    printf("========== FILE INFORMATION ==========\n\n");

    printf("Filename       : %s\n", filename);
    printf("File Size      : %.2f MB\n", file_size_mb);
    printf("Total Bytes    : %" PRIu64 "\n", total_bytes);
    printf("Buffer Size    : %d bytes\n", BUFFER_SIZE);
    printf("Total Chunks   : %" PRIu64 "\n", total_chunks);

    printf("\n====================================\n");
    printf("Status: SUCCESS\n");
    printf("====================================\n");

    return 0;
}