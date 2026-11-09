#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "tcp_client.h"


int tcp_client_run(
    const char *server_ip,
    unsigned short port
)
{
    SOCKET client_socket = INVALID_SOCKET;

    struct sockaddr_in server_address;

    const char *message = "Hello from C Song Parser Client!";

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

    // Send test message.
    int sent_bytes = send(
        client_socket,
        message,
        (int)strlen(message),
        0
    );

    if (sent_bytes == SOCKET_ERROR)
    {
        printf("[CLIENT ERROR] Could not send data.\n");
        printf("Error Code: %d\n", WSAGetLastError());

        closesocket(client_socket);

        return -1;
    }

    printf("[CLIENT] Sent %d bytes.\n", sent_bytes);

    // Close socket.
    closesocket(client_socket);

    printf("[CLIENT] Connection closed.\n");

    return 0;
}