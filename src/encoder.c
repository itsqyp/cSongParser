#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "encoder.h"
#include "sngp.h"
#include "checksum.h"

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
    const AudioMetadata *metadata,
    const unsigned char checksum[SHA256_HASH_SIZE]
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
   // Store SHA-256 checksum.

memcpy(
    header + SNGP_CHECKSUM_OFFSET,
    checksum,
    SNGP_CHECKSUM_SIZE
);
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
    unsigned char checksum[SHA256_HASH_SIZE];

    FILE *input = NULL;
    FILE *output = NULL;

    int result = 1;
    int output_created = 0;

    uint64_t total_copied = 0;
    uint64_t file_size = 0;

    if (input_filename == NULL || output_filename == NULL)
    {
        printf("[ENCODER ERROR] Invalid filename.\n");
        return 1;
    }

    /* Open original audio file. */

    input = fopen(input_filename, "rb");

    if (input == NULL)
    {
        perror("[ENCODER] Could not open input file");
        goto cleanup;
    }

    /* Determine original file size using 64-bit positioning. */

    if (_fseeki64(input, 0, SEEK_END) != 0)
    {
        perror("[ENCODER] Failed to seek input file");
        goto cleanup;
    }

    __int64 size = _ftelli64(input);

    if (size < 0)
    {
        perror("[ENCODER] Failed to determine file size");
        goto cleanup;
    }

    file_size = (uint64_t)size;

    if (_fseeki64(input, 0, SEEK_SET) != 0)
    {
        perror("[ENCODER] Failed to rewind input file");
        goto cleanup;
    }

    /* Calculate SHA-256 of the original audio payload. */

    if (checksum_sha256_file(input_filename, checksum) != 0)
    {
        printf("[ENCODER ERROR] Failed to calculate SHA-256.\n");
        goto cleanup;
    }

    /* Create destination container without overwriting existing files. */

    output = fopen(output_filename, "wbx");

    if (output == NULL)
    {
        perror("[ENCODER] Could not create output file");
        goto cleanup;
    }

    output_created = 1;

    /* Build SNGP header. */

    unsigned char header[SNGP_HEADER_SIZE];

    build_header(
        header,
        format,
        file_size,
        metadata,
        checksum
    );

    /* Write header. */

    if (fwrite(header, 1, sizeof(header), output) != sizeof(header))
    {
        perror("[ENCODER] Failed to write header");
        goto cleanup;
    }

    /* Copy original audio payload. */

    unsigned char buffer[4096];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, output) != bytes_read)
        {
            perror("[ENCODER] Failed to write payload");
            goto cleanup;
        }

        total_copied += bytes_read;
    }

    if (ferror(input))
    {
        perror("[ENCODER] Failed to read input file");
        goto cleanup;
    }

    /* Verify copied payload size. */

    if (total_copied != file_size)
    {
        printf("[ENCODER ERROR] Payload size verification failed.\n");
        goto cleanup;
    }

    /* Close input and check for errors. */

    if (fclose(input) != 0)
    {
        input = NULL;
        perror("[ENCODER] Failed to close input file");
        goto cleanup;
    }

    input = NULL;

    /* Finalize output container. */

    int close_result = fclose(output);
    output = NULL;

    if (close_result != 0)
    {
        perror("[ENCODER] Failed to finalize container");
        goto cleanup;
    }

    /* Everything succeeded. */

    result = 0;

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

cleanup:

    if (input != NULL)
    {
        fclose(input);
    }

    if (output != NULL)
    {
        fclose(output);
    }

    /*
     * Only remove the output if this invocation successfully
     * created it. Never delete a pre-existing container.
     */

    if (result != 0 && output_created)
    {
        if (remove(output_filename) != 0)
        {
            perror("[ENCODER] Failed to remove incomplete container");
        }
        else
        {
            printf("[ENCODER] Incomplete container removed.\n");
        }
    }

    return result;
}
