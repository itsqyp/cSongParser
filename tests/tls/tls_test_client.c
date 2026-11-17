#include <stdio.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "network/tls_context.h"
#include "network/tls_client.h"
#include "network/tls_io.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5555
#define BUFFER_SIZE 128

int main(void)
{
    WSADATA wsa_data;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        fprintf(stderr, "[TLS TEST] Winsock initialization failed.\n");
        return 1;
    }

    if (tls_initialize() != 0)
    {
        WSACleanup();
        return 1;
    }

    printf("\n====================================\n");
    printf("       TLS TEST CLIENT\n");
    printf("====================================\n");

    SSL_CTX *context = tls_create_client_context(
        "certs/sender/trusted-receiver-cert.pem"
    );

    if (context == NULL)
    {
        fprintf(stderr, "[TLS TEST] Failed to create client context.\n");
        WSACleanup();
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (client_socket == INVALID_SOCKET)
    {
        fprintf(stderr, "[TLS TEST] Socket creation failed.\n");

        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    InetPtonA(AF_INET, SERVER_IP, &server_address.sin_addr);

    printf("[TLS TEST] Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);

    if (connect(
            client_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) == SOCKET_ERROR)
    {
        fprintf(stderr, "[TLS TEST] Connection failed: %d\n", WSAGetLastError());

        closesocket(client_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("[TLS TEST] TCP connection established.\n");
    printf("[TLS TEST] Starting TLS handshake...\n");

    SSL *ssl = tls_client_connect(context, client_socket, SERVER_IP);

    if (ssl == NULL)
    {
        fprintf(stderr, "[TLS TEST] TLS connection or certificate verification failed.\n");

        closesocket(client_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("[TLS TEST] TLS handshake successful!\n");

    char message[BUFFER_SIZE] = "Hello from the TLS client!";

    if (tls_send_all(ssl, message, sizeof(message)) != 0)
    {
        fprintf(stderr, "[TLS TEST] Failed to send message.\n");

        tls_client_disconnect(ssl);
        closesocket(client_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("[TLS TEST] Message sent successfully.\n");

    char response[BUFFER_SIZE] = {0};

    if (tls_receive_all(ssl, response, sizeof(response)) != 0)
    {
        fprintf(stderr, "[TLS TEST] Failed to receive response.\n");

        tls_client_disconnect(ssl);
        closesocket(client_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("\n[TLS TEST] Server response:\n%s\n", response);

    tls_client_disconnect(ssl);

    closesocket(client_socket);

    tls_destroy_context(context);
    WSACleanup();

    printf("\n[TLS TEST] Client finished successfully.\n");

    return 0;
}