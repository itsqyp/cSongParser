#include <stdio.h>
#include <winsock2.h>

#include "tcp_io.h"


int tcp_send_all(
    SOCKET socket,
    const void *buffer,
    int length
)
{
    const char *data = (const char *)buffer;

    int total_sent = 0;

    while (total_sent < length)
    {
        int sent = send(
            socket,
            data + total_sent,
            length - total_sent,
            0
        );

        if (sent == SOCKET_ERROR)
        {
            printf("[NETWORK ERROR] Failed to send data.\n");
            printf("Error Code: %d\n", WSAGetLastError());

            return -1;
        }

        if (sent == 0)
        {
            printf("[NETWORK ERROR] Connection closed while sending.\n");

            return -1;
        }

        total_sent += sent;
    }

    return 0;
}


int tcp_receive_all(
    SOCKET socket,
    void *buffer,
    int length
)
{
    char *data = (char *)buffer;

    int total_received = 0;

    while (total_received < length)
    {
        int received = recv(
            socket,
            data + total_received,
            length - total_received,
            0
        );

        if (received == SOCKET_ERROR)
        {
            printf("[NETWORK ERROR] Failed to receive data.\n");
            printf("Error Code: %d\n", WSAGetLastError());

            return -1;
        }

        if (received == 0)
        {
            printf("[NETWORK ERROR] Connection closed while receiving.\n");

            return -1;
        }

        total_received += received;
    }

    return 0;
}