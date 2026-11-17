#ifndef TCP_FILE_TRANSFER_H
#define TCP_FILE_TRANSFER_H

#include <openssl/ssl.h>

int tcp_send_file(
    SSL *ssl,
    const char *filename
);

int tcp_receive_file(
    SSL *ssl,
    const char *output_filename
);

#endif