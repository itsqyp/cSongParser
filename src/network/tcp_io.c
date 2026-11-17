#include <stdio.h>
#include <stddef.h>

#include "tcp_io.h"
#include "tls_io.h"


int tcp_send_all(
    SSL *ssl,
    const void *buffer,
    int length
)
{
    if (ssl == NULL || buffer == NULL || length < 0)
    {
        printf("[NETWORK ERROR] Invalid send parameters.\n");
        return -1;
    }

    if (tls_send_all(
        ssl,
        buffer,
        (size_t)length
    ) != 0)
    {
        printf("[NETWORK ERROR] Failed to send TLS data.\n");
        return -1;
    }

    return 0;
}


int tcp_receive_all(
    SSL *ssl,
    void *buffer,
    int length
)
{
    if (ssl == NULL || buffer == NULL || length < 0)
    {
        printf("[NETWORK ERROR] Invalid receive parameters.\n");
        return -1;
    }

    if (tls_receive_all(
        ssl,
        buffer,
        (size_t)length
    ) != 0)
    {
        printf("[NETWORK ERROR] Failed to receive TLS data.\n");
        return -1;
    }

    return 0;
}