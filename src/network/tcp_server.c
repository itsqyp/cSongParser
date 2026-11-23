
#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>

#include "tcp_server.h"
#include "tcp_chunk_transfer.h"
#include "tcp_ack.h"
#include "reassembler.h"
#include "tls_context.h"
#include "tls_server.h"

#define SERVER_BACKLOG 5

int tcp_server_run(
    unsigned short port,
    const char *output_prefix
)
{
    if (output_prefix == NULL || output_prefix[0] == '\0')
    {
        printf("[SERVER] Invalid output prefix.\n");
        return -1;
    }

   SOCKET server_socket = INVALID_SOCKET;
   SOCKET client_socket = INVALID_SOCKET;

   SSL_CTX *tls_context = NULL;  
   SSL *tls_connection = NULL;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    int client_address_length = sizeof(client_address);
    int result = -1;

    char output_filename[512];

    uint32_t received_chunk_count = 0;

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
        goto cleanup;
    }

    /* Start listening. */
    if (listen(server_socket, SERVER_BACKLOG) == SOCKET_ERROR)
    {
        printf("[SERVER] Listen failed: %d\n", WSAGetLastError());
        goto cleanup;
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
        goto cleanup;
    }

    printf("[SERVER] Client connected.\n");
    /* Initialize TLS. */
if (tls_initialize() != 0)
{
    printf("[SERVER] TLS initialization failed.\n");
    goto cleanup;
}

tls_context = tls_create_server_context(
    "certs/receiver/receiver-cert.pem",
    "certs/receiver/receiver-key.pem"
);

if (tls_context == NULL)
{
    printf("[SERVER] Could not create TLS server context.\n");
    goto cleanup;
}

/* Establish encrypted connection. */
printf("[SERVER] Starting TLS handshake...\n");

tls_connection = tls_server_accept(
    tls_context,
    client_socket
);

if (tls_connection == NULL)
{
    printf("[SERVER] TLS handshake failed.\n");
    goto cleanup;
}

printf("[SERVER] Secure TLS connection established.\n");


    /* Receive all binary chunks. */
   if (tcp_receive_chunks(
    tls_connection,
    output_prefix,
    &received_chunk_count
) != 0)
    {
        printf("[SERVER] Chunk reception failed.\n");

        tcp_send_ack(tls_connection, TCP_ACK_FAILURE);

        goto cleanup;
    }

    printf("[SERVER] All %u chunks received successfully.\n",
           received_chunk_count);

    /* Generate reconstructed SNGP filename. */
    int filename_length = snprintf(
        output_filename,
        sizeof(output_filename),
        "%s.sngp",
        output_prefix
    );

    if (filename_length < 0 ||
        (size_t)filename_length >= sizeof(output_filename))
    {
        printf("[SERVER] Output filename is too long.\n");

        tcp_send_ack(tls_connection, TCP_ACK_FAILURE);

        goto cleanup;
    }

    /* Reassemble and validate received chunks. */
    printf("[SERVER] Starting chunk reassembly...\n");

    if (reassembler_combine_chunks(
        output_prefix,
        output_filename
    ) != 0)
    {
        printf("[SERVER] Chunk reassembly or validation failed.\n");

        tcp_send_ack(tls_connection, TCP_ACK_FAILURE);

        goto cleanup;
    }

    printf("[SERVER] SNGP container reconstructed successfully.\n");

    /* Send success acknowledgment only after validation. */
    if (tcp_send_ack(tls_connection, TCP_ACK_SUCCESS) != 0)
    {
        printf("[SERVER] Failed to send acknowledgment.\n");
        goto cleanup;
    }

    result = 0;

cleanup:

    if (tls_connection != NULL)
    {
        tls_server_disconnect(tls_connection);
        tls_connection = NULL;
    }

    if (client_socket != INVALID_SOCKET)
    {
        closesocket(client_socket);
    }

    if (server_socket != INVALID_SOCKET)
    {
        closesocket(server_socket);
    }

    if (tls_context != NULL)
    {
        tls_destroy_context(tls_context);
    }

    return result;
}
