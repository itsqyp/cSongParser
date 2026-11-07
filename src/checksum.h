#ifndef CHECKSUM_H
#define CHECKSUM_H

#define SHA256_HASH_SIZE 32

int checksum_sha256_file(
    const char *filename,
    unsigned char hash[SHA256_HASH_SIZE]
);

void checksum_print(
    const unsigned char hash[SHA256_HASH_SIZE]
);

#endif