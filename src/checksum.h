#ifndef CHECKSUM_H
#define CHECKSUM_H
#include <stdint.h>
#define SHA256_HASH_SIZE 32

int checksum_sha256_file(
    const char *filename,
    unsigned char hash[SHA256_HASH_SIZE]
);

int checksum_sha256_file_region(
    const char *filename,
    uint64_t offset,
    uint64_t length,
    unsigned char hash[SHA256_HASH_SIZE]
);


void checksum_print(
    const unsigned char hash[SHA256_HASH_SIZE]
);

#endif