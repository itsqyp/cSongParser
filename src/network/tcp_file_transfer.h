#ifndef TCP_CHUNK_TRANSFER_H
#define TCP_CHUNK_TRANSFER_H

#include <winsock2.h>

int tcp_send_chunks(
    SOCKET socket,
    const char *input_prefix
);

int tcp_receive_chunks(
    SOCKET socket,
    const char *output_prefix
);

#endif