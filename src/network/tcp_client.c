#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <stdint.h>
#include <stdlib.h>

#include "tcp_client.h"
#include "tcp_io.h"


#define TEST_MESSAGE_SIZE (64 * 1024)


int tcp_client_run(
    const char *server_ip,
    unsigned short port
)
{
    SOCKET client_socket = INVALID_SOCKET;

    struct sockaddr_in server_address;

    // Create TCP socket.
    client_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (client_socket == INVALID_SOCKET)
    {
        printf("[CLIENT ERROR] Could not create socket.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        return -1;
    }

    // Configure server address.
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to the server.
    printf("[CLIENT] Connecting to %s:%u...\n", server_ip, port);

    if (connect(
            client_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) == SOCKET_ERROR)
    {
        printf("[CLIENT ERROR] Could not connect to server.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        closesocket(client_socket);

        return -1;
    }

    printf("[CLIENT] Connected successfully!\n");

    // Allocate message memory.
    char *message = malloc(TEST_MESSAGE_SIZE);

    if (message == NULL)
    {
        printf("[CLIENT ERROR] Memory allocation failed.\n");

        closesocket(client_socket);

        return -1;
    }

    // Fill message with repeating alphabet pattern.
    for (int i = 0; i < TEST_MESSAGE_SIZE; i++)
    {
        message[i] = 'A' + (i % 26);
    }

    // Prepare message length.
    uint32_t network_message_length = htonl(TEST_MESSAGE_SIZE);

    // Send message header.
    if (tcp_send_all(
            client_socket,
            &network_message_length,
            sizeof(network_message_length)
        ) != 0)
    {
        free(message);
        closesocket(client_socket);

        return -1;
    }

    // Send complete message.
    if (tcp_send_all(
            client_socket,
            message,
            TEST_MESSAGE_SIZE
        ) != 0)
    {
        free(message);
        closesocket(client_socket);

        return -1;
    }

    printf(
        "[CLIENT] Successfully sent %d bytes.\n",
        TEST_MESSAGE_SIZE
    );

    // Release allocated memory.
    free(message);

    // Close socket.
    closesocket(client_socket);

    printf("[CLIENT] Connection closed.\n");

    return 0;
}