#ifndef TCP_IO_H
#define TCP_IO_H

#include <openssl/ssl.h>

int tcp_send_all(
    SSL *ssl,
    const void *buffer,
    int length
);

int tcp_receive_all(
    SSL *ssl,
    void *buffer,
    int length
);

#endif