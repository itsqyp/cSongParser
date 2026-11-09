#include <stdio.h>
#include <winsock2.h>

#include "tcp_client.h"
#include "tcp_file_transfer.h"

int tcp_client_run(
    const char *server_ip,
    unsigned short port,
    const char *filename
)
{
    SOCKET client_socket = INVALID_SOCKET;

    struct sockaddr_in server_address;

    /* Create TCP socket. */
    client_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (client_socket == INVALID_SOCKET)
    {
        printf("[CLIENT] Socket creation failed: %d\n", WSAGetLastError());
        return -1;
    }

    /* Configure server address. */
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(server_ip);

    /* Connect to server. */
    printf("[CLIENT] Connecting to %s:%u...\n", server_ip, port);

    if (connect(
        client_socket,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    ) == SOCKET_ERROR)
    {
        printf("[CLIENT] Connection failed: %d\n", WSAGetLastError());

        closesocket(client_socket);
        return -1;
    }

    printf("[CLIENT] Connected successfully.\n");

    /* Send binary file. */
    if (tcp_send_file(client_socket, filename) != 0)
    {
        printf("[CLIENT] File transfer failed.\n");

        closesocket(client_socket);
        return -1;
    }

    printf("[CLIENT] File transfer completed.\n");

    closesocket(client_socket);

    return 0;
}