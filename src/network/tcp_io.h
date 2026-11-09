#ifndef TCP_IO_H
#define TCP_IO_H

#include <winsock2.h>

int tcp_send_all(
    SOCKET socket,
    const void *buffer,
    int length
);

int tcp_receive_all(
    SOCKET socket,
    void *buffer,
    int length
);

#endif