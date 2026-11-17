#ifndef TCP_HANDSHAKE_H
#define TCP_HANDSHAKE_H

#include <stdint.h>
#include <openssl/ssl.h>

#include "format.h"

#define TCP_HANDSHAKE_MAGIC "SNGH"

#define TCP_HANDSHAKE_VERSION 1

#define TCP_HANDSHAKE_FILENAME_SIZE 256

#define TCP_HANDSHAKE_PACKET_SIZE 280

#define TCP_HANDSHAKE_ACCEPT 0
#define TCP_HANDSHAKE_REJECT 1

typedef struct
{
    char filename[TCP_HANDSHAKE_FILENAME_SIZE];

    uint64_t file_size;

    AudioFormat format;

} TcpFileInfo;


int tcp_send_handshake(
    SSL *ssl,
    const TcpFileInfo *info
);


int tcp_receive_handshake(
    SSL *ssl,
    TcpFileInfo *info
);


int tcp_send_handshake_response(
    SSL *ssl,
    unsigned char status
);


int tcp_receive_handshake_response(
    SSL *ssl
);

#endif