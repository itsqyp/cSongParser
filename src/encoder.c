#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "encoder.h"
#include "sngp.h"


// ============================================
// WRITE LITTLE-ENDIAN INTEGER
// ============================================

static void write_le16(unsigned char *buffer, uint16_t value)
{
    buffer[0] = (unsigned char)(value & 0xFF);
    buffer[1] = (unsigned char)((value >> 8) & 0xFF);
}


static void write_le32(unsigned char *buffer, uint32_t value)
{
    buffer[0] = (unsigned char)(value & 0xFF);
    buffer[1] = (unsigned char)((value >> 8) & 0xFF);
    buffer[2] = (unsigned char)((value >> 16) & 0xFF);
    buffer[3] = (unsigned char)((value >> 24) & 0xFF);
}


static void write_le64(unsigned char *buffer, uint64_t value)
{
    for (int i = 0; i < 8; i++)
    {
        buffer[i] = (unsigned char)((value >> (i * 8)) & 0xFF);
    }
}


// ============================================
// COPY TEXT INTO HEADER
// ============================================

static void write_text(
    unsigned char *header,
    size_t offset,
    size_t field_size,
    const char *text
)
{
    if (text == NULL)
    {
        return;
    }

    size_t length = strlen(text);

    if (length >= field_size)
    {
        length = field_size - 1;
    }

    memcpy(&header[offset], text, length);
}

// ============================================
// BUILD SNGP HEADER
// ============================================

static void build_header(
    unsigned char *header,
    AudioFormat format,
    uint64_t payload_size,
    const AudioMetadata *metadata
)
{
    memset(header, 0, SNGP_HEADER_SIZE);

    // Magic signature

    memcpy(header, SNGP_MAGIC, 4);

    // Header information

    write_le16(&header[4], SNGP_VERSION);
    write_le16(&header[6], SNGP_HEADER_SIZE);

    // Audio format

    write_le32(&header[8], (uint32_t)format);

    // Flags: bit 0 indicates an ID3v2 tag was present.

    uint32_t flags = 0;

    if (metadata != NULL && metadata->has_id3v2)
    {
        flags |= 1;
    }

    write_le32(&header[12], flags);

    // Original file size

    write_le64(&header[16], payload_size);

    if (metadata != NULL)
    {
        write_le32(&header[24], metadata->duration_seconds);
        write_le32(&header[28], metadata->sample_rate);
        write_le32(&header[32], metadata->bitrate);

        write_le16(&header[36], (uint16_t)metadata->channels);

        // Metadata text fields

        write_text(header, 40, SNGP_TITLE_SIZE, metadata->title);
        write_text(header, 72, SNGP_ARTIST_SIZE, metadata->artist);
        write_text(header, 104, SNGP_ALBUM_SIZE, metadata->album);
        write_text(header, 136, SNGP_ENCODING_SIZE, metadata->encoding);
    }
}



// ============================================
// CREATE SNGP CONTAINER
// ============================================

int encoder_create_container(
    const char *input_filename,
    const char *output_filename,
    AudioFormat format,
    const AudioMetadata *metadata
)
{
    if (input_filename == NULL || output_filename == NULL)
    {
        return 1;
    }

    // Open original audio file.

    FILE *input = fopen(input_filename, "rb");

    if (input == NULL)
    {
        perror("[ENCODER] Could not open input file");
        return 1;
    }

    // Determine original file size.

    if (fseek(input, 0, SEEK_END) != 0)
    {
        fclose(input);
        return 1;
    }

    long file_size = ftell(input);

    if (file_size < 0)
    {
        fclose(input);
        return 1;
    }

    rewind(input);

    // Open destination container.

    FILE *output = fopen(output_filename, "wb");

    if (output == NULL)
    {
        perror("[ENCODER] Could not create output file");
        fclose(input);
        return 1;
    }

    // Build header.

    unsigned char header[SNGP_HEADER_SIZE];

    build_header(
        header,
        format,
        (uint64_t)file_size,
        metadata
    );

    // Write header.

    if (fwrite(header, 1, sizeof(header), output) != sizeof(header))
    {
        perror("[ENCODER] Failed to write header");

        fclose(input);
        fclose(output);

        return 1;
    }

    // Copy original audio bytes.

    unsigned char buffer[4096];

    size_t bytes_read;

    uint64_t total_copied = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, output) != bytes_read)
        {
            perror("[ENCODER] Failed to write payload");

            fclose(input);
            fclose(output);

            return 1;
        }

        total_copied += bytes_read;
    }

    if (ferror(input))
    {
        perror("[ENCODER] Failed to read input file");

        fclose(input);
        fclose(output);

        return 1;
    }

    // Close files and verify close errors.

    int input_error = fclose(input);
    int output_error = fclose(output);

    if (input_error != 0 || output_error != 0)
    {
        printf("[ENCODER] Failed to finalize container.\n");
        return 1;
    }

    if (total_copied != (uint64_t)file_size)
    {
        printf("[ENCODER] Payload size verification failed.\n");
        return 1;
    }

    printf("\n====================================\n");
    printf("         SNGP ENCODER               \n");
    printf("====================================\n\n");

    printf("Input File      : %s\n", input_filename);
    printf("Output File     : %s\n", output_filename);
    printf("Header Size     : %d bytes\n", SNGP_HEADER_SIZE);
    printf("Payload Size    : %llu bytes\n",
           (unsigned long long)total_copied);

    printf("Container Size  : %llu bytes\n",
           (unsigned long long)(SNGP_HEADER_SIZE + total_copied));

    printf("\n[ENCODER] Container created successfully.\n");

    return 0;
}