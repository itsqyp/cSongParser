#include <stdio.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "network/tls_context.h"
#include "network/tls_server.h"
#include "network/tls_io.h"

#define TEST_PORT 5555
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
    printf("       TLS TEST SERVER\n");
    printf("====================================\n");

    SSL_CTX *context = tls_create_server_context(
        "certs/receiver/receiver-cert.pem",
        "certs/receiver/receiver-key.pem"
    );

    if (context == NULL)
    {
        fprintf(stderr, "[TLS TEST] Failed to create server context.\n");
        WSACleanup();
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_socket == INVALID_SOCKET)
    {
        fprintf(stderr, "[TLS TEST] Socket creation failed.\n");
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TEST_PORT);

    InetPtonA(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (bind(
            server_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) == SOCKET_ERROR)
    {
        fprintf(stderr, "[TLS TEST] Bind failed: %d\n", WSAGetLastError());

        closesocket(server_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 1) == SOCKET_ERROR)
    {
        fprintf(stderr, "[TLS TEST] Listen failed.\n");

        closesocket(server_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("[TLS TEST] Listening on 127.0.0.1:%d\n", TEST_PORT);
    printf("[TLS TEST] Waiting for client...\n");

    SOCKET client_socket = accept(server_socket, NULL, NULL);

    if (client_socket == INVALID_SOCKET)
    {
        fprintf(stderr, "[TLS TEST] Accept failed.\n");

        closesocket(server_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("[TLS TEST] Client connected.\n");
    printf("[TLS TEST] Starting TLS handshake...\n");

    SSL *ssl = tls_server_accept(context, client_socket);

    if (ssl == NULL)
    {
        fprintf(stderr, "[TLS TEST] TLS handshake failed.\n");

        closesocket(client_socket);
        closesocket(server_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("[TLS TEST] TLS handshake successful!\n");

    char received_message[BUFFER_SIZE] = {0};

    if (tls_receive_all(ssl, received_message, sizeof(received_message)) != 0)
    {
        fprintf(stderr, "[TLS TEST] Failed to receive message.\n");

        tls_server_disconnect(ssl);
        closesocket(client_socket);
        closesocket(server_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("\n[TLS TEST] Received message:\n%s\n", received_message);

    char response[BUFFER_SIZE] = "Hello from the TLS server!";

    if (tls_send_all(ssl, response, sizeof(response)) != 0)
    {
        fprintf(stderr, "[TLS TEST] Failed to send response.\n");

        tls_server_disconnect(ssl);
        closesocket(client_socket);
        closesocket(server_socket);
        tls_destroy_context(context);
        WSACleanup();
        return 1;
    }

    printf("[TLS TEST] Response sent successfully.\n");

    tls_server_disconnect(ssl);

    closesocket(client_socket);
    closesocket(server_socket);

    tls_destroy_context(context);
    WSACleanup();

    printf("\n[TLS TEST] Server finished successfully.\n");

    return 0;
}