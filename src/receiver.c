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

#include "decoder.h"
#include "reassembler.h"
#include "chunker.h"

#include "network/tcp_chunk_transfer.h"
#include "network/tcp_ack.h"

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


static void receiver_cleanup(
    const char *container_filename,
    const char *output_prefix,
    uint64_t container_size,
    int container_created,
    int chunks_created
)
{
    /* Delete only the container created by this transfer. */
    if (container_created)
    {
        remove(container_filename);
    }

    /* Delete chunks only if this transfer created them. */
    if (chunks_created)
    {
        uint64_t total_chunks =
            container_size / DEFAULT_CHUNK_SIZE +
            (container_size % DEFAULT_CHUNK_SIZE != 0);

        for (uint64_t i = 0; i < total_chunks; i++)
        {
            char chunk_filename[INPUT_SIZE];

            int length = snprintf(
                chunk_filename,
                sizeof(chunk_filename),
                "%s.part%06llu.sngc",
                output_prefix,
                (unsigned long long)i
            );

            if (length < 0 || length >= (int)sizeof(chunk_filename))
            {
                continue;
            }

            remove(chunk_filename);
        }
    }
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
    char container_filename[INPUT_SIZE];
char output_prefix[INPUT_SIZE];

uint64_t container_size = 0;

int transfer_accepted = 0;

int chunks_created = 0;
int container_created = 0;
int output_created = 0;

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

   /*
   Prepare temporary file paths.
*/

if (snprintf(
    output_prefix,
    sizeof(output_prefix),
    "%s.incoming",
    destination
) >= (int)sizeof(output_prefix) ||
    snprintf(
        container_filename,
        sizeof(container_filename),
        "%s.incoming.sngp",
        destination
    ) >= (int)sizeof(container_filename))
{
    printf("[RECEIVER ERROR] Destination path is too long.\n");

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

transfer_accepted = 1;

printf("\n[RECEIVER] Transfer accepted.\n");
printf("[RECEIVER] Destination: %s\n", destination);

/*
   Receive chunks.
*/

printf("\n[RECEIVER] Receiving chunks...\n");

if (tcp_receive_chunks(
    client_socket,
    output_prefix
) != 0)
{
    printf("[RECEIVER ERROR] Chunk reception failed.\n");
    goto transfer_finished;
}

chunks_created = 1;

/*
   Reassemble SNGC chunks.
*/

printf("\n[RECEIVER] Reassembling chunks...\n");

if (reassembler_combine_chunks(
    output_prefix,
    container_filename
) != 0)
{
    printf("[RECEIVER ERROR] Reassembly failed.\n");
    goto transfer_finished;
}

container_created = 1;

/*
   Determine reconstructed container size.
*/

FILE *container = fopen(container_filename, "rb");

if (container == NULL ||
    fseek(container, 0, SEEK_END) != 0)
{
    if (container != NULL)
    {
        fclose(container);
    }

    printf("[RECEIVER ERROR] Could not inspect reconstructed container.\n");
    goto transfer_finished;
}

long container_length = ftell(container);

fclose(container);

if (container_length <= 0)
{
    printf("[RECEIVER ERROR] Invalid reconstructed container size.\n");
    goto transfer_finished;
}

container_size = (uint64_t)container_length;

/*
   Decode original audio file.
*/

printf("\n[RECEIVER] Extracting original audio file...\n");

if (decoder_extract_payload(
    container_filename,
    destination
) != 0)
{
    printf("[RECEIVER ERROR] Audio extraction failed.\n");
    goto transfer_finished;
}

output_created = 1;

/*
   Verify original file size.
*/

FILE *output_file = fopen(destination, "rb");

if (output_file == NULL ||
    fseek(output_file, 0, SEEK_END) != 0)
{
    if (output_file != NULL)
    {
        fclose(output_file);
    }

    printf("[RECEIVER ERROR] Could not verify received file.\n");
    if (output_created)
{
    remove(destination);
    output_created = 0;
}
    goto transfer_finished;
}

long received_size = ftell(output_file);

fclose(output_file);

if (received_size < 0 ||
    (uint64_t)received_size != incoming_file.file_size)
{
    printf("[RECEIVER ERROR] Received file size mismatch.\n");

    if (output_created)
    {
        remove(destination);
        output_created = 0;
    }

    goto transfer_finished;
}

printf("\n====================================\n");
printf("       TRANSFER COMPLETED\n");
printf("====================================\n");

printf("[RECEIVER] Original audio file reconstructed.\n");
printf("[RECEIVER] Saved to: %s\n", destination);
printf("[RECEIVER] File size verified successfully.\n");

result = 0;

transfer_finished:

/*
   Send final acknowledgement.
*/

if (transfer_accepted)
{
    unsigned char ack_status =
        result == 0 ? TCP_ACK_SUCCESS : TCP_ACK_FAILURE;

    if (tcp_send_ack(client_socket, ack_status) != 0)
    {
        printf("[RECEIVER ERROR] Could not send final ACK.\n");
        result = 1;
    }
}

/*
   Clean up temporary files.
*/

   receiver_cleanup(
    container_filename,
    output_prefix,
    container_size,
    container_created,
    chunks_created
);

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