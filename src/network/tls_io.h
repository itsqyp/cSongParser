#ifndef TLS_IO_H
#define TLS_IO_H

#include <stddef.h>
#include <openssl/ssl.h>

/*
 * Send exactly the requested number of bytes over TLS.
 *
 * ssl: Active TLS connection.
 * buffer: Data to transmit.
 * length: Number of bytes to transmit.
 *
 * Returns:
 * 0 on success.
 * -1 on failure.
 */
int tls_send_all(
    SSL *ssl,
    const void *buffer,
    size_t length
);

/*
 * Receive exactly the requested number of bytes over TLS.
 *
 * ssl: Active TLS connection.
 * buffer: Destination buffer.
 * length: Number of bytes to receive.
 *
 * Returns:
 * 0 on success.
 * -1 on failure.
 */
int tls_receive_all(
    SSL *ssl,
    void *buffer,
    size_t length
);

#endif