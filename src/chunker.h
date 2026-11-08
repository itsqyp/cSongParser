#ifndef CHUNKER_H
#define CHUNKER_H

#include <stdint.h>

#define SNGC_MAGIC "SNGC"

#define SNGC_VERSION 1
#define SNGC_HEADER_SIZE 32

#define DEFAULT_CHUNK_SIZE (1024 * 1024)

int chunker_create_chunks(
    const char *input_filename,
    const char *output_prefix,
    uint32_t chunk_size
);

#endif