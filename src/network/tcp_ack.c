#include <stdio.h>
#include <string.h>

#include "tcp_ack.h"
#include "tcp_io.h"

/*
 * Send an 8-byte transfer acknowledgment.
 *
 * Protocol:
 * Bytes 0-3: SACK
 * Byte 4:    Status
 * Bytes 5-7: Reserved
 */
int tcp_send_ack(
    SOCKET socket,
    unsigned char status
)
{
    unsigned char acknowledgment[TCP_ACK_SIZE] = {0};

    if (status != TCP_ACK_SUCCESS &&
        status != TCP_ACK_FAILURE)
    {
        printf("[ACK] Invalid acknowledgment status.\n");
        return -1;
    }

    memcpy(acknowledgment, "SACK", 4);

    acknowledgment[4] = status;

    if (tcp_send_all(
        socket,
        acknowledgment,
        TCP_ACK_SIZE
    ) != 0)
    {
        printf("[ACK] Failed to send acknowledgment.\n");
        return -1;
    }

    printf("[ACK] Acknowledgment sent: %s\n",
        status == TCP_ACK_SUCCESS ? "SUCCESS" : "FAILURE");

    return 0;
}

/*
 * Receive and validate an 8-byte transfer acknowledgment.
 *
 * Returns:
 *  0 = Success
 *  1 = Server reported failure
 * -1 = Invalid acknowledgment or communication error
 */
int tcp_receive_ack(
    SOCKET socket
)
{
    unsigned char acknowledgment[TCP_ACK_SIZE];

    if (tcp_receive_all(
        socket,
        acknowledgment,
        TCP_ACK_SIZE
    ) != 0)
    {
        printf("[ACK] Failed to receive acknowledgment.\n");
        return -1;
    }

    /* Validate protocol identifier. */
    if (memcmp(acknowledgment, "SACK", 4) != 0)
    {
        printf("[ACK] Invalid acknowledgment identifier.\n");
        return -1;
    }

    /* Validate status. */
    if (acknowledgment[4] != TCP_ACK_SUCCESS &&
        acknowledgment[4] != TCP_ACK_FAILURE)
    {
        printf("[ACK] Invalid acknowledgment status.\n");
        return -1;
    }

    /* Reserved bytes must be zero. */
    if (acknowledgment[5] != 0 ||
        acknowledgment[6] != 0 ||
        acknowledgment[7] != 0)
    {
        printf("[ACK] Invalid reserved bytes.\n");
        return -1;
    }

    if (acknowledgment[4] == TCP_ACK_SUCCESS)
    {
        printf("[ACK] Server confirmed successful transfer.\n");
        return TCP_ACK_SUCCESS;
    }

    printf("[ACK] Server reported transfer failure.\n");

    return TCP_ACK_FAILURE;
}