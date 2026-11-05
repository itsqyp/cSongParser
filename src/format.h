#ifndef FORMAT_H
#define FORMAT_H

#include <stddef.h>

typedef enum
{
    FORMAT_UNKNOWN,
    FORMAT_MP3,
    FORMAT_WAV,
    FORMAT_FLAC,
    FORMAT_OGG,
    FORMAT_MP4,
    FORMAT_AAC,
    FORMAT_AIFF

} AudioFormat;

AudioFormat detect_audio_format(const unsigned char *data, size_t size);

const char *get_format_name(AudioFormat format);

#endif