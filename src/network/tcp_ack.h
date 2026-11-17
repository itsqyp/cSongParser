#ifndef TCP_ACK_H
#define TCP_ACK_H

#include <openssl/ssl.h>

#define TCP_ACK_SIZE 8

#define TCP_ACK_SUCCESS 0
#define TCP_ACK_FAILURE 1

int tcp_send_ack(
    SSL *ssl,
    unsigned char status
);

int tcp_receive_ack(
    SSL *ssl
);

#endif