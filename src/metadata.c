#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "metadata.h"


// ============================================
// HELPER: Read a 32-bit big-endian integer
// ============================================

static uint32_t read_be32(const unsigned char *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}


// ============================================
// HELPER: Read an ID3 synchsafe integer
// ============================================

static uint32_t read_synchsafe(const unsigned char *data)
{
    return ((uint32_t)(data[0] & 0x7F) << 21) |
           ((uint32_t)(data[1] & 0x7F) << 14) |
           ((uint32_t)(data[2] & 0x7F) << 7) |
           (uint32_t)(data[3] & 0x7F);
}


// ============================================
// HELPER: Decode ID3 text
// ============================================

static void decode_text(
    const unsigned char *data,
    size_t size,
    char *output,
    size_t output_size
)
{
    if (data == NULL || size < 2 || output_size == 0)
    {
        return;
    }

    unsigned char encoding = data[0];

    size_t input_index = 1;
    size_t output_index = 0;

    // UTF-8 or ISO-8859-1

    if (encoding == 0 || encoding == 3)
    {
        while (input_index < size &&
               data[input_index] != 0 &&
               output_index < output_size - 1)
        {
            output[output_index++] = (char)data[input_index++];
        }
    }

    // UTF-16

    else if (encoding == 1 || encoding == 2)
    {
        int little_endian = (encoding == 1);

        // Check for UTF-16 BOM

        if (input_index + 1 < size)
        {
            if (data[input_index] == 0xFF &&
                data[input_index + 1] == 0xFE)
            {
                little_endian = 1;
                input_index += 2;
            }

            else if (data[input_index] == 0xFE &&
                     data[input_index + 1] == 0xFF)
            {
                little_endian = 0;
                input_index += 2;
            }
        }

        while (input_index + 1 < size &&
               output_index < output_size - 1)
        {
            uint16_t character;

            if (little_endian)
            {
                character =
                    data[input_index] |
                    ((uint16_t)data[input_index + 1] << 8);
            }
            else
            {
                character =
                    ((uint16_t)data[input_index] << 8) |
                    data[input_index + 1];
            }

            input_index += 2;

            if (character == 0)
            {
                break;
            }

            // Preserve ASCII characters.
            // Non-ASCII UTF-16 characters are represented by '?'.

            output[output_index++] =
                character < 128 ? (char)character : '?';
        }
    }

    output[output_index] = '\0';
}


// ============================================
// ID3v2 METADATA EXTRACTION
// ============================================

static long extract_id3_metadata(
    FILE *file,
    AudioMetadata *metadata,
    long file_size
)
{
    unsigned char header[10];

    rewind(file);

    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        return 0;
    }

    // Check ID3 signature

    if (memcmp(header, "ID3", 3) != 0)
    {
        return 0;
    }

    unsigned char version = header[3];

    // Support ID3v2.3 and ID3v2.4

    if (version != 3 && version != 4)
    {
        printf("[METADATA] Unsupported ID3 version: 2.%u\n", version);
        return 0;
    }

    uint32_t tag_size = read_synchsafe(&header[6]);

    long tag_end = 10L + (long)tag_size;

    if (tag_end > file_size)
    {
        printf("[METADATA] Invalid ID3 tag size.\n");
        return 0;
    }

    metadata->has_id3v2 = 1;

    printf("[METADATA] ID3v2.%u tag detected.\n", version);

    // Extended headers and unsynchronized tags
    // will be handled in a later improvement.

    if (header[5] & 0xC0)
    {
        printf("[METADATA] Special ID3 flags detected; skipping tag frames.\n");
        return tag_end;
    }

    long position = 10;

    while (position + 10 <= tag_end)
    {
        unsigned char frame_header[10];

        if (fseek(file, position, SEEK_SET) != 0)
        {
            break;
        }

        if (fread(frame_header, 1, 10, file) != 10)
        {
            break;
        }

        // Padding: frames end when the frame ID is zero.

        if (frame_header[0] == 0)
        {
            break;
        }

        uint32_t frame_size;

        if (version == 4)
        {
            frame_size = read_synchsafe(&frame_header[4]);
        }
        else
        {
            frame_size = read_be32(&frame_header[4]);
        }

        if (frame_size == 0 ||
            frame_size > (uint32_t)(tag_end - position - 10))
        {
            break;
        }

        // Skip compressed or encrypted frames.

        if (frame_header[8] == 0 && frame_header[9] == 0)
        {
            unsigned char frame_data[1024];

            size_t bytes_to_read = frame_size;

            if (bytes_to_read > sizeof(frame_data))
            {
                bytes_to_read = sizeof(frame_data);
            }

            if (fread(frame_data, 1, bytes_to_read, file) != bytes_to_read)
            {
                break;
            }

            char *destination = NULL;

            if (memcmp(frame_header, "TIT2", 4) == 0)
            {
                destination = metadata->title;
            }

            else if (memcmp(frame_header, "TPE1", 4) == 0)
            {
                destination = metadata->artist;
            }

            else if (memcmp(frame_header, "TALB", 4) == 0)
            {
                destination = metadata->album;
            }

            if (destination != NULL)
            {
                decode_text(
                    frame_data,
                    bytes_to_read,
                    destination,
                    METADATA_TEXT_LENGTH
                );
            }
        }

        position += 10 + (long)frame_size;
    }

    return tag_end;
}


// ============================================
// MPEG AUDIO FRAME HEADER PARSING
// ============================================

static int parse_mpeg_header(
    const unsigned char *header,
    AudioMetadata *metadata
)
{
    // Check MPEG synchronization bits.

    if (header[0] != 0xFF ||
        (header[1] & 0xE0) != 0xE0)
    {
        return 0;
    }

    unsigned int version = (header[1] >> 3) & 0x03;
    unsigned int layer = (header[1] >> 1) & 0x03;

    unsigned int bitrate_index = (header[2] >> 4) & 0x0F;
    unsigned int sample_index = (header[2] >> 2) & 0x03;


// Debugging output for MPEG header parsing

//     printf(
//     "[DEBUG] MPEG Header: %02X %02X %02X %02X\n",
//     header[0],
//     header[1],
//     header[2],
//     header[3]
// );

// printf("[DEBUG] Version bits   : %u\n", version);
// printf("[DEBUG] Layer bits     : %u\n", layer);
// printf("[DEBUG] Bitrate index  : %u\n", bitrate_index);
// printf("[DEBUG] Sample index   : %u\n", sample_index);

    // Reject reserved values.

    if (version == 1 ||
        layer == 0 ||
        bitrate_index == 0 ||
        bitrate_index == 15 ||
        sample_index == 3)
    {
        return 0;
    }

    // Bitrate tables in kbps.

static const unsigned int mpeg1_bitrates[3][14] =
{
    // Layer I
    {32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},

    // Layer II
    {32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},

    // Layer III
    {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
};
static const unsigned int mpeg2_bitrates[3][14] =
{
    // Layer I
    {32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},

    // Layer II
    {8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},

    // Layer III
    {8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
};

    // Layer values:
    // 1 = Layer III
    // 2 = Layer II
    // 3 = Layer I

    unsigned int bitrate;

  if (version == 3)
{
    bitrate = mpeg1_bitrates[3 - layer][bitrate_index - 1];
}
else
{
    bitrate = mpeg2_bitrates[3 - layer][bitrate_index - 1];
}

if (bitrate == 0)
{
    return 0;
}
//debugging bitrate output
// printf("[DEBUG] Calculated bitrate: %u kbps\n", bitrate);

    static const unsigned int sample_rates[3] =
    {
        44100,
        48000,
        32000
    };

    unsigned int sample_rate = sample_rates[sample_index];

    if (version == 2)
    {
        sample_rate /= 2;
    }
    else if (version == 0)
    {
        sample_rate /= 4;
    }

    unsigned int channel_mode = (header[3] >> 6) & 0x03;

    metadata->bitrate = bitrate;
    metadata->sample_rate = sample_rate;
    metadata->channels = (channel_mode == 3) ? 1 : 2;

    const char *mpeg_version;

    if (version == 3)
    {
        mpeg_version = "1";
    }
    else if (version == 2)
    {
        mpeg_version = "2";
    }
    else
    {
        mpeg_version = "2.5";
    }

    unsigned int actual_layer = 4 - layer;

snprintf(
    metadata->encoding,
    sizeof(metadata->encoding),
    "MPEG-%s Layer %u",
    mpeg_version,
    actual_layer
);

    return 1;
}


// ============================================
// FIND FIRST VALID MPEG FRAME
// ============================================

static int find_mpeg_frame(
    FILE *file,
    long start_position,
    AudioMetadata *metadata
)
{
    if (fseek(file, start_position, SEEK_SET) != 0)
    {
        return 0;
    }

    unsigned char header[4];

    int byte;

    // Read sequentially until a valid frame is found.

    while ((byte = fgetc(file)) != EOF)
    {
        if (byte != 0xFF)
        {
            continue;
        }

        header[0] = 0xFF;

        if (fread(&header[1], 1, 3, file) != 3)
        {
            return 0;
        }

        if (parse_mpeg_header(header, metadata))
        {
            return 1;
        }

        // Move back three bytes to continue scanning.

        if (fseek(file, -3, SEEK_CUR) != 0)
        {
            return 0;
        }
    }

    return 0;
}


// ============================================
// MAIN METADATA EXTRACTION FUNCTION
// ============================================

int metadata_extract(
    const char *filename,
    AudioMetadata *metadata
)
{
    if (filename == NULL || metadata == NULL)
    {
        return 1;
    }

    memset(metadata, 0, sizeof(AudioMetadata));

    strcpy(metadata->title, "Unknown");
    strcpy(metadata->artist, "Unknown");
    strcpy(metadata->album, "Unknown");
    strcpy(metadata->encoding, "Unknown");

    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        perror("[METADATA] Could not open file");
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 1;
    }

    long file_size = ftell(file);

    if (file_size < 0)
    {
        fclose(file);
        return 1;
    }

    // Extract descriptive ID3 information.

    long audio_start = extract_id3_metadata(
        file,
        metadata,
        file_size
    );

    // Extract technical MPEG information.

    if (!find_mpeg_frame(file, audio_start, metadata))
    {
        printf("[METADATA] No valid MPEG audio frame found.\n");
        fclose(file);
        return 1;
    }

    // Approximate duration using file size and bitrate.

    if (metadata->bitrate > 0)
    {
        uint64_t duration =
            ((uint64_t)file_size * 8) /
            ((uint64_t)metadata->bitrate * 1000);

        metadata->duration_seconds = (uint32_t)duration;
    }

    fclose(file);

    return 0;
}


// ============================================
// DISPLAY EXTRACTED METADATA
// ============================================

void metadata_print(const AudioMetadata *metadata)
{
    if (metadata == NULL)
    {
        return;
    }

    printf("\n====================================\n");
    printf("         AUDIO METADATA             \n");
    printf("====================================\n\n");

    printf("Title          : %s\n", metadata->title);
    printf("Artist         : %s\n", metadata->artist);
    printf("Album          : %s\n", metadata->album);

    printf("Duration       : %u seconds\n", metadata->duration_seconds);

    printf("Sample Rate    : %u Hz\n", metadata->sample_rate);
    printf("Channels       : %u\n", metadata->channels);
    printf("Bitrate        : %u kbps\n", metadata->bitrate);

    printf("Encoding       : %s\n", metadata->encoding);

    printf("ID3v2 Tag      : %s\n",
           metadata->has_id3v2 ? "Present" : "Not found");

    printf("\n====================================\n");
}