#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#include <winsock2.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include "tls_client.h"


/*
 * Establish a TLS connection over an existing TCP socket.
 */
SSL *tls_client_connect(
    SSL_CTX *context,
    SOCKET socket,
    const char *server_ip
)
{
    if (context == NULL ||
        socket == INVALID_SOCKET ||
        server_ip == NULL ||
        server_ip[0] == '\0')
    {
        fprintf(stderr, "[TLS CLIENT ERROR] Invalid connection parameters.\n");
        return NULL;
    }

    /*
     * OpenSSL's socket BIO interface accepts an int descriptor.
     * Reject values that cannot safely be represented.
     */
    if ((uintptr_t)socket > INT_MAX)
    {
        fprintf(stderr, "[TLS CLIENT ERROR] Socket handle exceeds OpenSSL's descriptor range.\n");
        return NULL;
    }

    /*
     * Create SSL connection object.
     */
    SSL *ssl = SSL_new(context);

    if (ssl == NULL)
    {
        fprintf(stderr, "[TLS CLIENT ERROR] Could not create SSL object.\n");

        ERR_print_errors_fp(stderr);

        return NULL;
    }

    /*
     * Verify that the certificate belongs to the expected server IP.
     */
    X509_VERIFY_PARAM *verify_parameters = SSL_get0_param(ssl);

    if (X509_VERIFY_PARAM_set1_ip_asc(
            verify_parameters,
            server_ip
        ) != 1)
    {
        fprintf(stderr, "[TLS CLIENT ERROR] Could not configure server IP verification.\n");

        ERR_print_errors_fp(stderr);

        SSL_free(ssl);

        return NULL;
    }

    /*
     * Attach the connected Winsock socket.
     */
    if (SSL_set_fd(ssl, (int)socket) != 1)
    {
        fprintf(stderr, "[TLS CLIENT ERROR] Could not attach socket to SSL object.\n");

        ERR_print_errors_fp(stderr);

        SSL_free(ssl);

        return NULL;
    }

    printf("[TLS CLIENT] Starting TLS handshake with %s...\n", server_ip);

    /*
     * Perform TLS handshake.
     */
    int handshake_result = SSL_connect(ssl);

    if (handshake_result != 1)
    {
        int error_code = SSL_get_error(ssl, handshake_result);

        fprintf(
            stderr,
            "[TLS CLIENT ERROR] TLS handshake failed. OpenSSL error: %d\n",
            error_code
        );

        ERR_print_errors_fp(stderr);

        SSL_free(ssl);

        return NULL;
    }

    /*
     * Explicitly verify certificate validation result.
     */
    long verification_result = SSL_get_verify_result(ssl);

    if (verification_result != X509_V_OK)
    {
        fprintf(
            stderr,
            "[TLS CLIENT ERROR] Server certificate verification failed: %s\n",
            X509_verify_cert_error_string(verification_result)
        );

        SSL_shutdown(ssl);
        SSL_free(ssl);

        return NULL;
    }

    printf("[TLS CLIENT] TLS connection established successfully.\n");

    printf(
        "[TLS CLIENT] Protocol: %s\n",
        SSL_get_version(ssl)
    );

    printf(
        "[TLS CLIENT] Cipher: %s\n",
        SSL_get_cipher_name(ssl)
    );

    printf("[TLS CLIENT] Server certificate verified successfully.\n");

    return ssl;
}


/*
 * Gracefully shut down a TLS connection.
 */
void tls_client_disconnect(SSL *ssl)
{
    if (ssl == NULL)
    {
        return;
    }

    /*
     * Attempt TLS shutdown.
     */
    int result = SSL_shutdown(ssl);

    if (result == 0)
    {
        /*
         * The first call sent close_notify.
         * The second call waits for the peer's close_notify.
         */
        SSL_shutdown(ssl);
    }

    SSL_free(ssl);

    printf("[TLS CLIENT] TLS connection closed.\n");
}