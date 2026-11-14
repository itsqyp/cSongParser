#include <stdio.h>
#include <stdint.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tls_io.h"


/*
 * Send exactly the requested number of bytes over TLS.
 */
int tls_send_all(
    SSL *ssl,
    const void *buffer,
    size_t length
)
{
    if (ssl == NULL || (buffer == NULL && length > 0))
    {
        fprintf(stderr, "[TLS IO ERROR] Invalid send parameters.\n");
        return -1;
    }

    const unsigned char *data = buffer;
    size_t total_sent = 0;

    while (total_sent < length)
    {
        size_t bytes_sent = 0;

        int result = SSL_write_ex(
            ssl,
            data + total_sent,
            length - total_sent,
            &bytes_sent
        );

        if (result == 1)
        {
            if (bytes_sent == 0)
            {
                fprintf(stderr, "[TLS IO ERROR] TLS write made no progress.\n");
                return -1;
            }

            total_sent += bytes_sent;

            continue;
        }

        /*
         * SSL_get_error must immediately follow the failed SSL operation.
         */
        int error_code = SSL_get_error(ssl, result);

        switch (error_code)
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                /*
                 * OpenSSL needs the socket to become readable or writable
                 * before this operation can continue.
                 *
                 * Our current sockets are blocking sockets.
                 */
                continue;

            case SSL_ERROR_ZERO_RETURN:
                fprintf(stderr, "[TLS IO ERROR] Peer closed the TLS connection.\n");
                return -1;

            default:
                fprintf(
                    stderr,
                    "[TLS IO ERROR] TLS write failed. Error: %d\n",
                    error_code
                );

                ERR_print_errors_fp(stderr);

                return -1;
        }
    }

    return 0;
}


/*
 * Receive exactly the requested number of bytes over TLS.
 */
int tls_receive_all(
    SSL *ssl,
    void *buffer,
    size_t length
)
{
    if (ssl == NULL || (buffer == NULL && length > 0))
    {
        fprintf(stderr, "[TLS IO ERROR] Invalid receive parameters.\n");
        return -1;
    }

    unsigned char *data = buffer;
    size_t total_received = 0;

    while (total_received < length)
    {
        size_t bytes_received = 0;

        int result = SSL_read_ex(
            ssl,
            data + total_received,
            length - total_received,
            &bytes_received
        );

        if (result == 1)
        {
            if (bytes_received == 0)
            {
                fprintf(stderr, "[TLS IO ERROR] TLS read made no progress.\n");
                return -1;
            }

            total_received += bytes_received;

            continue;
        }

        /*
         * SSL_get_error must immediately follow the failed SSL operation.
         */
        int error_code = SSL_get_error(ssl, result);

        switch (error_code)
        {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                continue;

            case SSL_ERROR_ZERO_RETURN:
                fprintf(
                    stderr,
                    "[TLS IO ERROR] Peer closed the TLS connection.\n"
                );

                return -1;

            default:
                fprintf(
                    stderr,
                    "[TLS IO ERROR] TLS read failed. Error: %d\n",
                    error_code
                );

                ERR_print_errors_fp(stderr);

                return -1;
        }
    }

    return 0;
}