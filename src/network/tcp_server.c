#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <stdint.h>
#include <stdlib.h>


#include "tcp_server.h"
#include "tcp_io.h"

#define SERVER_BACKLOG 5
#define MAX_MESSAGE_SIZE (1024 * 1024)



int tcp_server_run(unsigned short port)
{
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    int client_address_length = sizeof(client_address);

   
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
   // Receive message length.
uint32_t network_message_length;

if (tcp_receive_all(
        client_socket,
        &network_message_length,
        sizeof(network_message_length)
    ) != 0)
{
    closesocket(client_socket);
    closesocket(server_socket);

    return -1;
}

// Convert length from network byte order.
uint32_t message_length = ntohl(network_message_length);

// Validate message length.

if (message_length == 0 || message_length > MAX_MESSAGE_SIZE)
{
    printf("[SERVER ERROR] Invalid message length.\n");

    closesocket(client_socket);
    closesocket(server_socket);

    return -1;
}

// Allocate memory for the message.
char *message = malloc((size_t)message_length + 1);

if (message == NULL)
{
    printf("[SERVER ERROR] Memory allocation failed.\n");

    closesocket(client_socket);
    closesocket(server_socket);

    return -1;
}

// Receive the complete message.
if (tcp_receive_all(
        client_socket,
        message,
        (int)message_length
    ) != 0)
{
    free(message);

    closesocket(client_socket);
    closesocket(server_socket);

    return -1;
}

message[message_length] = '\0';

printf("\n[SERVER] Received complete message!\n");
printf("[SERVER] Received %u bytes.\n", message_length);

printf("[SERVER] First 32 bytes: %.32s\n", message);

// Release allocated memory.
free(message);

    // Close sockets.
    closesocket(client_socket);
    closesocket(server_socket);

    printf("\n[SERVER] Connection closed.\n");

    return 0;
}