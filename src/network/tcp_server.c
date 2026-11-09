#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "tcp_server.h"


#define SERVER_BACKLOG 5
#define BUFFER_SIZE 1024


int tcp_server_run(unsigned short port)
{
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    int client_address_length = sizeof(client_address);

    char buffer[BUFFER_SIZE];

    // Create TCP socket.
    server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (server_socket == INVALID_SOCKET)
    {
        printf("[SERVER ERROR] Could not create socket.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        return -1;
    }

    // Configure server address.
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket to port.
    if (bind(
            server_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) == SOCKET_ERROR)
    {
        printf("[SERVER ERROR] Could not bind socket.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        closesocket(server_socket);

        return -1;
    }

    // Start listening for connections.
    if (listen(server_socket, SERVER_BACKLOG) == SOCKET_ERROR)
    {
        printf("[SERVER ERROR] Could not start listening.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        closesocket(server_socket);

        return -1;
    }

    printf("\n====================================\n");
    printf("          TCP SERVER\n");
    printf("====================================\n");

    printf("[SERVER] Listening on port %u...\n", port);
    printf("[SERVER] Waiting for a client connection...\n");

    // Accept an incoming client connection.
    client_socket = accept(
        server_socket,
        (struct sockaddr *)&client_address,
        &client_address_length
    );

    if (client_socket == INVALID_SOCKET)
    {
        printf("[SERVER ERROR] Could not accept connection.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        closesocket(server_socket);

        return -1;
    }

    printf("[SERVER] Client connected successfully!\n");

    // Receive a test message.
    int received_bytes = recv(
        client_socket,
        buffer,
        BUFFER_SIZE - 1,
        0
    );

    if (received_bytes == SOCKET_ERROR)
    {
        printf("[SERVER ERROR] Could not receive data.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        closesocket(client_socket);
        closesocket(server_socket);

        return -1;
    }

    if (received_bytes == 0)
    {
        printf("[SERVER] Client disconnected.\n");
    }
    else
    {
        buffer[received_bytes] = '\0';

        printf("\n[SERVER] Received message:\n");
        printf("%s\n", buffer);
    }

    // Close sockets.
    closesocket(client_socket);
    closesocket(server_socket);

    printf("\n[SERVER] Connection closed.\n");

    return 0;
}