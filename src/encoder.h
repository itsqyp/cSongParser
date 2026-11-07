#ifndef ENCODER_H
#define ENCODER_H

#include "format.h"
#include "metadata.h"

int encoder_create_container(
    const char *input_filename,
    const char *output_filename,
    AudioFormat format,
    const AudioMetadata *metadata
);

#endif