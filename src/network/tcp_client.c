
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "tcp_client.h"
#include "tcp_chunk_transfer.h"
#include "tcp_ack.h"
#include "tls_context.h"
#include "tls_client.h"




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

  SSL_CTX *tls_context = NULL; 
  SSL *tls_connection = NULL;

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

   tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);
return -1;
}

    printf("[CLIENT] Connected successfully.\n");
    /* Initialize TLS. */
if (tls_initialize() != 0)
{
    printf("[CLIENT ERROR] TLS initialization failed.\n");
    tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);
return -1;
}

tls_context = tls_create_client_context(
    "certs/sender/trusted-receiver-cert.pem"
);

if (tls_context == NULL)
{
    printf("[CLIENT ERROR] Could not create TLS client context.\n");
   tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);
return -1;
}

/* Establish encrypted connection. */
printf("[CLIENT] Starting TLS handshake...\n");

tls_connection = tls_client_connect(
    tls_context,
    client_socket,
    server_ip
);

if (tls_connection == NULL)
{
    printf("[CLIENT ERROR] TLS connection failed.\n");

    tls_destroy_context(tls_context);
   tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);
return -1;
}

printf("[CLIENT] Secure TLS connection established.\n");

    /* Automatically discover and send chunks. */
   if (tcp_send_chunks(
    tls_connection,
    input_prefix
) != 0)
    {
        printf("[CLIENT] Chunk transfer failed.\n");

       tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);
return -1;
    }

    /* Wait for server acknowledgment. */
printf("[CLIENT] Waiting for server acknowledgment...\n");

int acknowledgment = tcp_receive_ack(tls_connection);

if (acknowledgment == TCP_ACK_FAILURE)
{
    printf("[CLIENT] Server rejected the transfer.\n");

  tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);
return -1;
}

if (acknowledgment != TCP_ACK_SUCCESS)
{
    printf("[CLIENT] Invalid or missing server acknowledgment.\n");

   tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);
return -1;
}

printf("[CLIENT] Server confirmed successful transfer.\n");
printf("[CLIENT] Transfer completed successfully.\n");

tls_client_disconnect(tls_connection);
tls_destroy_context(tls_context);
closesocket(client_socket);

return 0;
}
