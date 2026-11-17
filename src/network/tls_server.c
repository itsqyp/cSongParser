#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#include <winsock2.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tls_server.h"
 

/*
 * Establish a TLS connection over an accepted TCP socket.
 */
SSL *tls_server_accept(
    SSL_CTX *context,
    SOCKET client_socket
)
{
    if (context == NULL || client_socket == INVALID_SOCKET)
    {
        fprintf(stderr, "[TLS SERVER ERROR] Invalid connection parameters.\n");
        return NULL;
    }

    /*
     * OpenSSL's SSL_set_fd interface accepts an int descriptor.
     * Reject handles that cannot safely be represented.
     */
    if ((uintptr_t)client_socket > INT_MAX)
    {
        fprintf(stderr, "[TLS SERVER ERROR] Socket handle exceeds OpenSSL's descriptor range.\n");
        return NULL;
    }

    /*
     * Create SSL connection object.
     */
    SSL *ssl = SSL_new(context);

    if (ssl == NULL)
    {
        fprintf(stderr, "[TLS SERVER ERROR] Could not create SSL object.\n");

        ERR_print_errors_fp(stderr);

        return NULL;
    }

    /*
     * Attach the accepted Winsock socket.
     *
     * SSL_set_fd does not take ownership of the socket.
     * The caller remains responsible for closesocket().
     */
    if (SSL_set_fd(ssl, (int)client_socket) != 1)
    {
        fprintf(stderr, "[TLS SERVER ERROR] Could not attach socket to SSL object.\n");

        ERR_print_errors_fp(stderr);

        SSL_free(ssl);

        return NULL;
    }

    printf("[TLS SERVER] Waiting for TLS handshake...\n");

    /*
     * Perform server-side TLS handshake.
     */
    int handshake_result = SSL_accept(ssl);

    if (handshake_result != 1)
    {
        int error_code = SSL_get_error(ssl, handshake_result);

        fprintf(
            stderr,
            "[TLS SERVER ERROR] TLS handshake failed. OpenSSL error: %d\n",
            error_code
        );

        ERR_print_errors_fp(stderr);

        SSL_free(ssl);

        return NULL;
    }

    printf("[TLS SERVER] TLS connection established successfully.\n");

    printf(
        "[TLS SERVER] Protocol: %s\n",
        SSL_get_version(ssl)
    );

    printf(
        "[TLS SERVER] Cipher: %s\n",
        SSL_get_cipher_name(ssl)
    );

    return ssl;
}


/*
 * Shut down a TLS connection.
 */
void tls_server_disconnect(SSL *ssl)
{
    if (ssl == NULL)
    {
        return;
    }

    /*
     * Send TLS close_notify.
     *
     * The socket itself is not closed here.
     */
    int result = SSL_shutdown(ssl);

    if (result < 0)
    {
        int error_code = SSL_get_error(ssl, result);

        if (error_code != SSL_ERROR_ZERO_RETURN)
        {
            fprintf(
                stderr,
                "[TLS SERVER WARNING] TLS shutdown returned error: %d\n",
                error_code
            );

            ERR_print_errors_fp(stderr);
        }
    }

    SSL_free(ssl);

    printf("[TLS SERVER] TLS connection closed.\n");
}