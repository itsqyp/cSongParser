#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "sender.h"
#include "format.h"
#include "metadata.h"
#include "encoder.h"
#include "chunker.h"

#include "network/socket_utils.h"
#include "network/tcp_io.h"
#include "network/tcp_handshake.h"
#include "network/tcp_chunk_transfer.h"
#include "network/tcp_ack.h"

#define INPUT_SIZE 512

static int sender_read_input(
    const char *prompt,
    char *buffer,
    size_t buffer_size
)
{
    printf("%s", prompt);

    if (fgets(buffer, (int)buffer_size, stdin) == NULL)
    {
        return -1;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';

    if (buffer[0] == '\0')
    {
        printf("[SENDER ERROR] Input cannot be empty.\n");
        return -1;
    }

    return 0;
}

static int sender_read_port(unsigned short *port)
{
    char input[INPUT_SIZE];
    char *end_pointer;

    if (sender_read_input(
        "Enter receiver port: ",
        input,
        sizeof(input)
    ) != 0)
    {
        return -1;
    }

    unsigned long value = strtoul(input, &end_pointer, 10);

    if (*end_pointer != '\0' ||
        value == 0 ||
        value > 65535)
    {
        printf("[SENDER ERROR] Invalid port number.\n");
        return -1;
    }

    *port = (unsigned short)value;

    return 0;
}

static const char *sender_get_filename(const char *path)
{
    const char *filename = path;
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');

    if (slash != NULL && slash + 1 > filename)
    {
        filename = slash + 1;
    }

    if (backslash != NULL && backslash + 1 > filename)
    {
        filename = backslash + 1;
    }

    return filename;
}

static int sender_validate_file(
    const char *filename,
    AudioFormat *format,
    uint64_t *file_size,
    AudioMetadata *metadata
)
{
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("[SENDER ERROR] Could not open input file.\n");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return -1;
    }

    long size = ftell(file);

    if (size <= 0)
    {
        fclose(file);
        printf("[SENDER ERROR] Invalid or empty input file.\n");
        return -1;
    }

    *file_size = (uint64_t)size;

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return -1;
    }

    unsigned char buffer[4096];

    size_t bytes_read = fread(
        buffer,
        1,
        sizeof(buffer),
        file
    );

    fclose(file);

    if (bytes_read == 0)
    {
        printf("[SENDER ERROR] Could not read input file.\n");
        return -1;
    }

    *format = detect_audio_format(buffer, bytes_read);

    if (*format != FORMAT_MP3)
    {
        printf("[SENDER ERROR] Only MP3 files are currently supported.\n");
        return -1;
    }

    if (metadata_extract(filename, metadata) != 0)
    {
        printf("[SENDER ERROR] Could not extract audio metadata.\n");
        return -1;
    }

    return 0;
}

static void sender_cleanup(
    const char *container_filename,
    const char *output_prefix,
    uint64_t container_size
)
{
    remove(container_filename);

    uint64_t total_chunks =
        container_size / DEFAULT_CHUNK_SIZE +
        (container_size % DEFAULT_CHUNK_SIZE != 0);

    for (uint64_t i = 0; i < total_chunks; i++)
    {
        char chunk_filename[INPUT_SIZE];

        snprintf(
            chunk_filename,
            sizeof(chunk_filename),
            "%s.part%06llu.sngc",
            output_prefix,
            (unsigned long long)i
        );

        remove(chunk_filename);
    }
}

int sender_run(void)
{
    SOCKET client_socket = INVALID_SOCKET;

    int result = 1;
    int winsock_initialized = 0;

    unsigned short port;

    char server_ip[INPUT_SIZE];
    char input_filename[INPUT_SIZE];

    char container_filename[INPUT_SIZE];
    char output_prefix[INPUT_SIZE];

    AudioFormat format;
    AudioMetadata metadata;

    uint64_t file_size;

    TcpFileInfo file_info;

    struct sockaddr_in server_address;

    printf("\n====================================\n");
    printf("           FILE SENDER\n");
    printf("====================================\n");

    /*
       Collect receiver information.
    */

    if (sender_read_input(
        "Enter receiver IPv4 address: ",
        server_ip,
        sizeof(server_ip)
    ) != 0)
    {
        return 1;
    }

    if (sender_read_port(&port) != 0)
    {
        return 1;
    }

    if (sender_read_input(
        "Enter audio file path: ",
        input_filename,
        sizeof(input_filename)
    ) != 0)
    {
        return 1;
    }

    /*
       Validate input file.
    */

    printf("\n[SENDER] Validating audio file...\n");

    if (sender_validate_file(
        input_filename,
        &format,
        &file_size,
        &metadata
    ) != 0)
    {
        return 1;
    }

    printf("[SENDER] Audio file validated successfully.\n");

    /*
       Prepare temporary filenames.
    */

    if (snprintf(
        container_filename,
        sizeof(container_filename),
        "%s.transfer.sngp",
        input_filename
    ) >= (int)sizeof(container_filename) ||
        snprintf(
            output_prefix,
            sizeof(output_prefix),
            "%s.transfer",
            input_filename
        ) >= (int)sizeof(output_prefix))
    {
        printf("[SENDER ERROR] Input file path is too long.\n");
        return 1;
    }

    /*
       Encode original MP3 into SNGP.
    */

    printf("\n[SENDER] Creating SNGP container...\n");

    if (encoder_create_container(
        input_filename,
        container_filename,
        format,
        &metadata
    ) != 0)
    {
        printf("[SENDER ERROR] Encoding failed.\n");
        return 1;
    }

    FILE *container = fopen(container_filename, "rb");

    if (container == NULL ||
        fseek(container, 0, SEEK_END) != 0)
    {
        if (container != NULL)
        {
            fclose(container);
        }

        remove(container_filename);

        printf("[SENDER ERROR] Could not inspect container.\n");
        return 1;
    }

    long container_length = ftell(container);
    fclose(container);

    if (container_length <= 0)
    {
        remove(container_filename);
        return 1;
    }

    uint64_t container_size = (uint64_t)container_length;

    /*
       Create SNGC chunks.
    */

    printf("\n[SENDER] Creating transfer chunks...\n");

    if (chunker_create_chunks(
        container_filename,
        output_prefix,
        DEFAULT_CHUNK_SIZE
    ) != 0)
    {
        sender_cleanup(
            container_filename,
            output_prefix,
            container_size
        );

        return 1;
    }

    /*
       Initialize Winsock.
    */

    if (network_initialize() != 0)
    {
        sender_cleanup(
            container_filename,
            output_prefix,
            container_size
        );

        return 1;
    }

    winsock_initialized = 1;

    /*
       Create TCP socket.
    */

    client_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (client_socket == INVALID_SOCKET)
    {
        printf("[SENDER ERROR] Could not create socket.\n");
        goto cleanup;
    }

    /*
       Configure receiver address.
    */

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (InetPtonA(
        AF_INET,
        server_ip,
        &server_address.sin_addr
    ) != 1)
    {
        printf("[SENDER ERROR] Invalid IPv4 address.\n");
        goto cleanup;
    }

    /*
       Connect to receiver.
    */

    printf("\n[SENDER] Connecting to %s:%u...\n", server_ip, port);

    if (connect(
        client_socket,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    ) == SOCKET_ERROR)
    {
        printf("[SENDER ERROR] Could not connect to receiver.\n");
        goto cleanup;
    }

    printf("[SENDER] Connected successfully.\n");

    /*
       Prepare handshake information.
    */

    memset(&file_info, 0, sizeof(file_info));

    const char *basename = sender_get_filename(input_filename);

    if (strlen(basename) >= sizeof(file_info.filename))
    {
        printf("[SENDER ERROR] Filename is too long.\n");
        goto cleanup;
    }

    strcpy(file_info.filename, basename);

    file_info.file_size = file_size;
    file_info.format = format;

    /*
       Send handshake.
    */

    printf("\n[SENDER] Sending transfer request...\n");

    if (tcp_send_handshake(
        client_socket,
        &file_info
    ) != 0)
    {
        printf("[SENDER ERROR] Could not send handshake.\n");
        goto cleanup;
    }

    /*
       Wait for receiver acceptance.
    */

    if (tcp_receive_handshake_response(client_socket) != 0)
    {
        printf("[SENDER] Receiver rejected the transfer.\n");
        goto cleanup;
    }

    printf("[SENDER] Receiver accepted the transfer.\n");

    /*
       Transfer chunks.
    */

    printf("\n[SENDER] Transferring chunks...\n");

    if (tcp_send_chunks(
        client_socket,
        output_prefix
    ) != 0)
    {
        printf("[SENDER ERROR] Chunk transfer failed.\n");
        goto cleanup;
    }

    /*
       Wait for final receiver acknowledgement.
    */

    printf("\n[SENDER] Waiting for final verification...\n");

    if (tcp_receive_ack(client_socket) != 0)
    {
        printf("[SENDER ERROR] Final transfer acknowledgement failed.\n");
        goto cleanup;
    }

    printf("\n====================================\n");
    printf("       TRANSFER COMPLETED\n");
    printf("====================================\n");

    printf("[SENDER] File transferred successfully.\n");

    result = 0;

cleanup:

    if (client_socket != INVALID_SOCKET)
    {
        closesocket(client_socket);
    }

    if (winsock_initialized)
    {
        network_cleanup();
    }

    sender_cleanup(
        container_filename,
        output_prefix,
        container_size
    );

    return result;
}