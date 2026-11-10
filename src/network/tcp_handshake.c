#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "tcp_handshake.h"
#include "tcp_io.h"


#define TCP_HANDSHAKE_RESPONSE_SIZE 8
#define TCP_HANDSHAKE_RESPONSE_MAGIC "SHRS"

#define TCP_HANDSHAKE_FILENAME_OFFSET 24


/* Encode a 16-bit integer in big-endian order. */
static void encode_u16_be(
    uint16_t value,
    unsigned char buffer[2]
)
{
    buffer[0] = (unsigned char)(value >> 8);
    buffer[1] = (unsigned char)(value & 0xFF);
}


/* Decode a 16-bit big-endian integer. */
static uint16_t decode_u16_be(
    const unsigned char buffer[2]
)
{
    return (uint16_t)(
        ((uint16_t)buffer[0] << 8) |
        buffer[1]
    );
}


/* Encode a 32-bit integer in big-endian order. */
static void encode_u32_be(
    uint32_t value,
    unsigned char buffer[4]
)
{
    for (int i = 3; i >= 0; i--)
    {
        buffer[i] = (unsigned char)(value & 0xFF);
        value >>= 8;
    }
}


/* Decode a 32-bit big-endian integer. */
static uint32_t decode_u32_be(
    const unsigned char buffer[4]
)
{
    uint32_t value = 0;

    for (int i = 0; i < 4; i++)
    {
        value = (value << 8) | buffer[i];
    }

    return value;
}


/* Encode a 64-bit integer in big-endian order. */
static void encode_u64_be(
    uint64_t value,
    unsigned char buffer[8]
)
{
    for (int i = 7; i >= 0; i--)
    {
        buffer[i] = (unsigned char)(value & 0xFF);
        value >>= 8;
    }
}


/* Decode a 64-bit big-endian integer. */
static uint64_t decode_u64_be(
    const unsigned char buffer[8]
)
{
    uint64_t value = 0;

    for (int i = 0; i < 8; i++)
    {
        value = (value << 8) | buffer[i];
    }

    return value;
}


/* Validate that a filename is a safe basename. */
static int validate_filename(
    const char *filename,
    size_t length
)
{
    if (filename == NULL || length == 0 ||
        length >= TCP_HANDSHAKE_FILENAME_SIZE)
    {
        return -1;
    }

    if (strcmp(filename, ".") == 0 ||
        strcmp(filename, "..") == 0)
    {
        return -1;
    }

    for (size_t i = 0; i < length; i++)
    {
        unsigned char character = (unsigned char)filename[i];

        if (character < 32 ||
            character == 127 ||
            character == '/' ||
            character == '\\' ||
            character == ':')
        {
            return -1;
        }
    }

    return 0;
}


/* Validate supported audio formats. */
static int validate_format(uint32_t format)
{
    return format >= FORMAT_MP3 &&
           format <= FORMAT_AIFF;
}


/*
 * Send file information to the receiver.
 *
 * Packet layout:
 *
 * Bytes 0-3:     SNGH
 * Bytes 4-5:     Protocol version
 * Bytes 6-7:     Filename length
 * Bytes 8-15:    Original file size
 * Bytes 16-19:   Audio format
 * Bytes 20-23:   Reserved
 * Bytes 24-279:  Filename
 */
int tcp_send_handshake(
    SOCKET socket,
    const TcpFileInfo *info
)
{
    if (info == NULL)
    {
        printf("[HANDSHAKE ERROR] Invalid file information.\n");
        return -1;
    }

    size_t filename_length = strlen(info->filename);

    if (validate_filename(info->filename, filename_length) != 0)
    {
        printf("[HANDSHAKE ERROR] Invalid filename.\n");
        return -1;
    }

    if (info->file_size == 0)
    {
        printf("[HANDSHAKE ERROR] Invalid file size.\n");
        return -1;
    }

    if (!validate_format((uint32_t)info->format))
    {
        printf("[HANDSHAKE ERROR] Unsupported audio format.\n");
        return -1;
    }

    unsigned char packet[TCP_HANDSHAKE_PACKET_SIZE] = {0};

    memcpy(packet, TCP_HANDSHAKE_MAGIC, 4);

    encode_u16_be(
        TCP_HANDSHAKE_VERSION,
        packet + 4
    );

    encode_u16_be(
        (uint16_t)filename_length,
        packet + 6
    );

    encode_u64_be(
        info->file_size,
        packet + 8
    );

    encode_u32_be(
        (uint32_t)info->format,
        packet + 16
    );

    memcpy(
        packet + TCP_HANDSHAKE_FILENAME_OFFSET,
        info->filename,
        filename_length
    );

    if (tcp_send_all(
        socket,
        packet,
        TCP_HANDSHAKE_PACKET_SIZE
    ) != 0)
    {
        printf("[HANDSHAKE ERROR] Failed to send file information.\n");
        return -1;
    }

    printf("[HANDSHAKE] File information sent.\n");

    return 0;
}


/*
 * Receive and validate file information.
 */
int tcp_receive_handshake(
    SOCKET socket,
    TcpFileInfo *info
)
{
    if (info == NULL)
    {
        printf("[HANDSHAKE ERROR] Invalid output structure.\n");
        return -1;
    }

    unsigned char packet[TCP_HANDSHAKE_PACKET_SIZE];

    if (tcp_receive_all(
        socket,
        packet,
        TCP_HANDSHAKE_PACKET_SIZE
    ) != 0)
    {
        printf("[HANDSHAKE ERROR] Failed to receive file information.\n");
        return -1;
    }

    /* Validate protocol identifier. */
    if (memcmp(packet, TCP_HANDSHAKE_MAGIC, 4) != 0)
    {
        printf("[HANDSHAKE ERROR] Invalid protocol identifier.\n");
        return -1;
    }

    /* Validate protocol version. */
    uint16_t version = decode_u16_be(packet + 4);

    if (version != TCP_HANDSHAKE_VERSION)
    {
        printf("[HANDSHAKE ERROR] Unsupported protocol version.\n");
        return -1;
    }

    /* Validate filename length. */
    uint16_t filename_length = decode_u16_be(packet + 6);

    if (filename_length == 0 ||
        filename_length >= TCP_HANDSHAKE_FILENAME_SIZE)
    {
        printf("[HANDSHAKE ERROR] Invalid filename length.\n");
        return -1;
    }

    /* Validate file size. */
    uint64_t file_size = decode_u64_be(packet + 8);

    if (file_size == 0)
    {
        printf("[HANDSHAKE ERROR] Invalid file size.\n");
        return -1;
    }

    /* Validate audio format. */
    uint32_t format = decode_u32_be(packet + 16);

    if (!validate_format(format))
    {
        printf("[HANDSHAKE ERROR] Unsupported audio format.\n");
        return -1;
    }

    /* Reserved bytes must be zero. */
    if (packet[20] != 0 ||
        packet[21] != 0 ||
        packet[22] != 0 ||
        packet[23] != 0)
    {
        printf("[HANDSHAKE ERROR] Invalid reserved bytes.\n");
        return -1;
    }

    /* Ensure filename field is properly terminated. */
    if (packet[TCP_HANDSHAKE_FILENAME_OFFSET + filename_length] != 0)
    {
        printf("[HANDSHAKE ERROR] Filename is not terminated.\n");
        return -1;
    }

    /* Remaining filename bytes must be zero. */
    for (size_t i = TCP_HANDSHAKE_FILENAME_OFFSET + filename_length + 1;
         i < TCP_HANDSHAKE_PACKET_SIZE;
         i++)
    {
        if (packet[i] != 0)
        {
            printf("[HANDSHAKE ERROR] Invalid filename padding.\n");
            return -1;
        }
    }

    char filename[TCP_HANDSHAKE_FILENAME_SIZE];

    memcpy(
        filename,
        packet + TCP_HANDSHAKE_FILENAME_OFFSET,
        filename_length
    );

    filename[filename_length] = '\0';

    if (validate_filename(filename, filename_length) != 0)
    {
        printf("[HANDSHAKE ERROR] Unsafe filename received.\n");
        return -1;
    }

    /* Store validated information. */
    memset(info, 0, sizeof(*info));

    memcpy(
        info->filename,
        filename,
        filename_length + 1
    );

    info->file_size = file_size;
    info->format = (AudioFormat)format;

    printf("[HANDSHAKE] File information received.\n");

    return 0;
}


/*
 * Send ACCEPT or REJECT.
 *
 * Packet layout:
 *
 * Bytes 0-3: SHRS
 * Byte 4:    Status
 * Bytes 5-7: Reserved
 */
int tcp_send_handshake_response(
    SOCKET socket,
    unsigned char status
)
{
    if (status != TCP_HANDSHAKE_ACCEPT &&
        status != TCP_HANDSHAKE_REJECT)
    {
        printf("[HANDSHAKE ERROR] Invalid response status.\n");
        return -1;
    }

    unsigned char response[TCP_HANDSHAKE_RESPONSE_SIZE] = {0};

    memcpy(
        response,
        TCP_HANDSHAKE_RESPONSE_MAGIC,
        4
    );

    response[4] = status;

    if (tcp_send_all(
        socket,
        response,
        TCP_HANDSHAKE_RESPONSE_SIZE
    ) != 0)
    {
        printf("[HANDSHAKE ERROR] Failed to send response.\n");
        return -1;
    }

    printf("[HANDSHAKE] Response sent: %s\n",
        status == TCP_HANDSHAKE_ACCEPT ? "ACCEPT" : "REJECT");

    return 0;
}


/*
 * Receive the receiver's handshake response.
 *
 * Returns:
 *  0 = Accepted
 *  1 = Rejected
 * -1 = Communication or validation error
 */
int tcp_receive_handshake_response(
    SOCKET socket
)
{
    unsigned char response[TCP_HANDSHAKE_RESPONSE_SIZE];

    if (tcp_receive_all(
        socket,
        response,
        TCP_HANDSHAKE_RESPONSE_SIZE
    ) != 0)
    {
        printf("[HANDSHAKE ERROR] Failed to receive response.\n");
        return -1;
    }

    if (memcmp(
        response,
        TCP_HANDSHAKE_RESPONSE_MAGIC,
        4
    ) != 0)
    {
        printf("[HANDSHAKE ERROR] Invalid response identifier.\n");
        return -1;
    }

    if (response[4] != TCP_HANDSHAKE_ACCEPT &&
        response[4] != TCP_HANDSHAKE_REJECT)
    {
        printf("[HANDSHAKE ERROR] Invalid response status.\n");
        return -1;
    }

    if (response[5] != 0 ||
        response[6] != 0 ||
        response[7] != 0)
    {
        printf("[HANDSHAKE ERROR] Invalid reserved bytes.\n");
        return -1;
    }

    if (response[4] == TCP_HANDSHAKE_ACCEPT)
    {
        printf("[HANDSHAKE] Receiver accepted the file.\n");
        return TCP_HANDSHAKE_ACCEPT;
    }

    printf("[HANDSHAKE] Receiver rejected the file.\n");

    return TCP_HANDSHAKE_REJECT;
}