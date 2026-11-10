#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "receiver.h"
#include "format.h"

#include "network/socket_utils.h"
#include "network/tcp_handshake.h"

#define INPUT_SIZE 512

static int receiver_read_port(unsigned short *port)
{
    char input[INPUT_SIZE];
    char *end_pointer;

    printf("\nEnter listening port: ");

    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        return -1;
    }

    unsigned long value = strtoul(input, &end_pointer, 10);

    while (*end_pointer == ' ' ||
           *end_pointer == '\t' ||
           *end_pointer == '\r' ||
           *end_pointer == '\n')
    {
        end_pointer++;
    }

    if (input[0] == '\n' ||
        *end_pointer != '\0' ||
        value == 0 ||
        value > 65535)
    {
        printf("[RECEIVER ERROR] Invalid port number.\n");
        return -1;
    }

    *port = (unsigned short)value;

    return 0;
}

static int receiver_read_destination(
    char *destination,
    size_t destination_size
)
{
    printf("\nEnter destination file path: ");

    if (fgets(destination, (int)destination_size, stdin) == NULL)
    {
        return -1;
    }

    destination[strcspn(destination, "\r\n")] = '\0';

    if (destination[0] == '\0')
    {
        printf("[RECEIVER ERROR] Destination cannot be empty.\n");
        return -1;
    }

    /*
       Check whether the destination already exists.
       We must never overwrite an existing file.
    */

    FILE *existing = fopen(destination, "rb");

    if (existing != NULL)
    {
        fclose(existing);

        printf("[RECEIVER ERROR] Destination file already exists.\n");
        return -1;
    }

    return 0;
}

static int receiver_display_file_info(const TcpFileInfo *info)
{
    printf("\n====================================\n");
    printf("         INCOMING FILE\n");
    printf("====================================\n");

    printf("Filename      : %s\n", info->filename);

    printf("File Size     : %llu bytes\n",
           (unsigned long long)info->file_size);

    printf("Audio Format  : %s\n",
           get_format_name(info->format));

    printf("====================================\n");

    if (info->format != FORMAT_MP3)
    {
        printf("\n[RECEIVER] This audio format is not supported yet.\n");
        return -1;
    }

    if (info->file_size == 0)
    {
        printf("\n[RECEIVER] Invalid incoming file size.\n");
        return -1;
    }

    return 0;
}

int receiver_run(void)
{
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    unsigned short port;

    int result = 1;
    int winsock_initialized = 0;

    struct sockaddr_in server_address;

    TcpFileInfo incoming_file;

    char destination[INPUT_SIZE];

    printf("\n====================================\n");
    printf("          FILE RECEIVER\n");
    printf("====================================\n");

    /*
       Read listening port.
    */

    if (receiver_read_port(&port) != 0)
    {
        return 1;
    }

    /*
       Initialize Winsock.
    */

    if (network_initialize() != 0)
    {
        return 1;
    }

    winsock_initialized = 1;

    /*
       Create TCP socket.
    */

    server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (server_socket == INVALID_SOCKET)
    {
        printf("[RECEIVER ERROR] Could not create socket.\n");
        goto cleanup;
    }

    /*
       Configure server address.
    */

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    /*
       Bind socket.
    */

    if (bind(
        server_socket,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    ) == SOCKET_ERROR)
    {
        printf("[RECEIVER ERROR] Could not bind port %u.\n", port);
        goto cleanup;
    }

    /*
       Start listening.
    */

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("[RECEIVER ERROR] Could not start listening.\n");
        goto cleanup;
    }

    printf("\n[RECEIVER] Listening on port %u...\n", port);
    printf("[RECEIVER] Waiting for sender connection...\n");

    /*
       Accept incoming connection.
    */

    client_socket = accept(
        server_socket,
        NULL,
        NULL
    );

    if (client_socket == INVALID_SOCKET)
    {
        printf("[RECEIVER ERROR] Could not accept connection.\n");
        goto cleanup;
    }

    printf("\n[RECEIVER] Sender connected!\n");

    /*
       Receive sender handshake.
    */

    if (tcp_receive_handshake(
        client_socket,
        &incoming_file
    ) != 0)
    {
        printf("[RECEIVER ERROR] Invalid sender handshake.\n");
        goto cleanup;
    }

    /*
       Display incoming file information.
    */

    if (receiver_display_file_info(&incoming_file) != 0)
    {
        tcp_send_handshake_response(
            client_socket,
            TCP_HANDSHAKE_REJECT
        );

        goto cleanup;
    }

    /*
       Ask where the original audio file should be saved.
    */

    if (receiver_read_destination(
        destination,
        sizeof(destination)
    ) != 0)
    {
        tcp_send_handshake_response(
            client_socket,
            TCP_HANDSHAKE_REJECT
        );

        goto cleanup;
    }

    /*
       Accept transfer.
    */

    if (tcp_send_handshake_response(
        client_socket,
        TCP_HANDSHAKE_ACCEPT
    ) != 0)
    {
        printf("[RECEIVER ERROR] Could not send handshake response.\n");
        goto cleanup;
    }

    printf("\n[RECEIVER] Transfer accepted.\n");
    printf("[RECEIVER] Destination: %s\n", destination);

    printf("\n[RECEIVER] Handshake completed successfully.\n");

    /*
       Chunk reception and file reconstruction
       will be implemented in the next stage.
    */

    result = 0;

cleanup:

    if (client_socket != INVALID_SOCKET)
    {
        closesocket(client_socket);
    }

    if (server_socket != INVALID_SOCKET)
    {
        closesocket(server_socket);
    }

    if (winsock_initialized)
    {
        network_cleanup();
    }

    return result;
}