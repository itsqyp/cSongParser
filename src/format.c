#include "format.h"

AudioFormat detect_audio_format(const unsigned char *data, size_t size)
{
    if (data == NULL || size < 4)
    {
        return FORMAT_UNKNOWN;
    }

    // MP3: ID3 metadata signature

    if (data[0] == 0x49 &&
        data[1] == 0x44 &&
        data[2] == 0x33)
    {
        return FORMAT_MP3;
    }

    // MP3: MPEG audio frame synchronization

    if (data[0] == 0xFF &&
        (data[1] & 0xE0) == 0xE0)
    {
        unsigned int version = (data[1] >> 3) & 0x03;
        unsigned int layer = (data[1] >> 1) & 0x03;

        if (version != 1 && layer != 0)
        {
            return FORMAT_MP3;
        }
    }

    // WAV: RIFF .... WAVE

    if (data[0] == 'R' &&
        data[1] == 'I' &&
        data[2] == 'F' &&
        data[3] == 'F')
    {
        if (size >= 12 &&
            data[8] == 'W' &&
            data[9] == 'A' &&
            data[10] == 'V' &&
            data[11] == 'E')
        {
            return FORMAT_WAV;
        }
    }

    // FLAC

    if (data[0] == 'f' &&
        data[1] == 'L' &&
        data[2] == 'a' &&
        data[3] == 'C')
    {
        return FORMAT_FLAC;
    }

    // OGG

    if (data[0] == 'O' &&
        data[1] == 'g' &&
        data[2] == 'g' &&
        data[3] == 'S')
    {
        return FORMAT_OGG;
    }

    // MP4 / M4A: ftyp at offset 4

    if (size >= 8 &&
        data[4] == 'f' &&
        data[5] == 't' &&
        data[6] == 'y' &&
        data[7] == 'p')
    {
        return FORMAT_MP4;
    }

    // AAC: ADTS frame synchronization

    if (data[0] == 0xFF &&
        (data[1] & 0xF6) == 0xF0)
    {
        return FORMAT_AAC;
    }

    // AIFF / AIFC

    if (data[0] == 'F' &&
        data[1] == 'O' &&
        data[2] == 'R' &&
        data[3] == 'M')
    {
        if (size >= 12 &&
            ((data[8] == 'A' &&
              data[9] == 'I' &&
              data[10] == 'F' &&
              data[11] == 'F') ||

             (data[8] == 'A' &&
              data[9] == 'I' &&
              data[10] == 'F' &&
              data[11] == 'C')))
        {
            return FORMAT_AIFF;
        }
    }

    return FORMAT_UNKNOWN;
}


const char *get_format_name(AudioFormat format)
{
    switch (format)
    {
        case FORMAT_MP3:
            return "MP3";

        case FORMAT_WAV:
            return "WAV";

        case FORMAT_FLAC:
            return "FLAC";

        case FORMAT_OGG:
            return "OGG";

        case FORMAT_MP4:
            return "MP4 / M4A";

        case FORMAT_AAC:
            return "AAC";

        case FORMAT_AIFF:
            return "AIFF";

        default:
            return "UNKNOWN";
    }
}