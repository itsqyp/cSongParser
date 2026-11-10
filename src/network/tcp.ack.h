#ifndef TCP_ACK_H
#define TCP_ACK_H

#include <winsock2.h>

#define TCP_ACK_SIZE 8

#define TCP_ACK_SUCCESS 0
#define TCP_ACK_FAILURE 1

int tcp_send_ack(
    SOCKET socket,
    unsigned char status
);

int tcp_receive_ack(
    SOCKET socket
);

#endif