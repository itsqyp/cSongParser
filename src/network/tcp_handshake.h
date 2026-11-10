#ifndef TCP_HANDSHAKE_H
#define TCP_HANDSHAKE_H

#include <stdint.h>
#include <winsock2.h>

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
    SOCKET socket,
    const TcpFileInfo *info
);


int tcp_receive_handshake(
    SOCKET socket,
    TcpFileInfo *info
);


int tcp_send_handshake_response(
    SOCKET socket,
    unsigned char status
);


int tcp_receive_handshake_response(
    SOCKET socket
);

#endif