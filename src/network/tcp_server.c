#include <stdio.h>
#include <winsock2.h>

#include "tcp_server.h"
#include "tcp_file_transfer.h"

#define SERVER_BACKLOG 5

int tcp_server_run(
    unsigned short port,
    const char *output_filename
)
{
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    int client_address_length = sizeof(client_address);

    /* Create TCP socket. */
    server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (server_socket == INVALID_SOCKET)
    {
        printf("[SERVER] Socket creation failed: %d\n", WSAGetLastError());
        return -1;
    }

    /* Configure server address. */
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind socket. */
    if (bind(
        server_socket,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    ) == SOCKET_ERROR)
    {
        printf("[SERVER] Bind failed: %d\n", WSAGetLastError());

        closesocket(server_socket);
        return -1;
    }

    /* Start listening. */
    if (listen(server_socket, SERVER_BACKLOG) == SOCKET_ERROR)
    {
        printf("[SERVER] Listen failed: %d\n", WSAGetLastError());

        closesocket(server_socket);
        return -1;
    }

    printf("[SERVER] Listening on port %u...\n", port);
    printf("[SERVER] Waiting for incoming connection...\n");

    /* Accept client connection. */
    client_socket = accept(
        server_socket,
        (struct sockaddr *)&client_address,
        &client_address_length
    );

    if (client_socket == INVALID_SOCKET)
    {
        printf("[SERVER] Accept failed: %d\n", WSAGetLastError());

        closesocket(server_socket);
        return -1;
    }

    printf("[SERVER] Client connected.\n");

    /* Receive binary file. */
    if (tcp_receive_file(client_socket, output_filename) != 0)
    {
        printf("[SERVER] File transfer failed.\n");

        closesocket(client_socket);
        closesocket(server_socket);
        return -1;
    }

    printf("[SERVER] File received successfully.\n");

    closesocket(client_socket);
    closesocket(server_socket);

    return 0;
}