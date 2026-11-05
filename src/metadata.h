#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>

#define METADATA_TEXT_LENGTH 256

typedef struct
{
    char title[METADATA_TEXT_LENGTH];
    char artist[METADATA_TEXT_LENGTH];
    char album[METADATA_TEXT_LENGTH];

    uint32_t duration_seconds;
    uint32_t sample_rate;
    uint32_t bitrate;
    uint32_t channels;

    char encoding[32];

    int has_id3v2;

} AudioMetadata;


int metadata_extract(const char *filename, AudioMetadata *metadata);

void metadata_print(const AudioMetadata *metadata);

#endif