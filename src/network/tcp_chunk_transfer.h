#ifndef TCP_CHUNK_TRANSFER_H
#define TCP_CHUNK_TRANSFER_H

#include <stdint.h>
#include <openssl/ssl.h>

int tcp_send_chunks(
    SSL *ssl,
    const char *input_prefix
);

int tcp_receive_chunks(
    SSL *ssl,
    const char *output_prefix,
    uint32_t *received_chunk_count
);

#endif