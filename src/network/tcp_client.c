
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "tcp_client.h"
#include "tcp_chunk_transfer.h"
#include "tcp_ack.h"




static const char *get_connection_error_message(int error_code)
{
    switch (error_code)
    {
        case WSAECONNREFUSED:
            return "Connection refused. Is the server running?";

        case WSAETIMEDOUT:
            return "Connection timed out.";

        case WSAEHOSTUNREACH:
            return "The server host is unreachable.";

        case WSAENETUNREACH:
            return "The network is unreachable.";

        case WSAEADDRNOTAVAIL:
            return "The requested address is unavailable.";

        case WSAEHOSTDOWN:
            return "The server host is down.";

        default:
            return "An unknown connection error occurred.";
    }
}

int tcp_client_run(
    const char *server_ip,
    unsigned short port,
    const char *input_prefix
)
{
    SOCKET client_socket = INVALID_SOCKET;

    struct sockaddr_in server_address = {0};

    /* Validate server IPv4 address. */
if (server_ip == NULL || server_ip[0] == '\0')
{
    printf("[CLIENT ERROR] Server IP address is empty.\n");
    return -1;
}

int address_result = InetPtonA(
    AF_INET,
    server_ip,
    &server_address.sin_addr
);

if (address_result == 0)
{
    printf("[CLIENT ERROR] Invalid IPv4 address: %s\n", server_ip);
    return -1;
}

if (address_result == SOCKET_ERROR)
{
    printf("[CLIENT ERROR] IPv4 validation failed: %d\n", WSAGetLastError());
    return -1;
}

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

    /* Connect to server. */
    printf("[CLIENT] Connecting to %s:%u...\n", server_ip, port);

   if (connect(
    client_socket,
    (struct sockaddr *)&server_address,
    sizeof(server_address)
) == SOCKET_ERROR)
{
    int error_code = WSAGetLastError();

    printf("[CLIENT ERROR] %s\n",
        get_connection_error_message(error_code));

    printf("[CLIENT ERROR] Windows socket error: %d\n", error_code);

    closesocket(client_socket);
    return -1;
}

    printf("[CLIENT] Connected successfully.\n");

    /* Automatically discover and send chunks. */
    if (tcp_send_chunks(
        client_socket,
        input_prefix
    ) != 0)
    {
        printf("[CLIENT] Chunk transfer failed.\n");

        closesocket(client_socket);
        return -1;
    }

    /* Wait for server acknowledgment. */
printf("[CLIENT] Waiting for server acknowledgment...\n");

int acknowledgment = tcp_receive_ack(client_socket);

if (acknowledgment == TCP_ACK_FAILURE)
{
    printf("[CLIENT] Server rejected the transfer.\n");

    closesocket(client_socket);
    return -1;
}

if (acknowledgment != TCP_ACK_SUCCESS)
{
    printf("[CLIENT] Invalid or missing server acknowledgment.\n");

    closesocket(client_socket);
    return -1;
}

printf("[CLIENT] Server confirmed successful transfer.\n");
printf("[CLIENT] Transfer completed successfully.\n");

closesocket(client_socket);

return 0;
}
