#include <stdio.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "tls_context.h"


/*
 * Initialize OpenSSL.
 */
int tls_initialize(void)
{
    if (OPENSSL_init_ssl(0, NULL) != 1)
    {
        fprintf(stderr, "[TLS ERROR] OpenSSL initialization failed.\n");

        ERR_print_errors_fp(stderr);

        return -1;
    }

    printf("[TLS] OpenSSL initialized successfully.\n");

    return 0;
}


/*
 * Apply common TLS configuration.
 */
static int tls_configure_context(SSL_CTX *context)
{
    if (context == NULL)
    {
        return -1;
    }

    /*
     * Require TLS 1.2 or newer.
     */
    if (SSL_CTX_set_min_proto_version(
            context,
            TLS1_2_VERSION
        ) != 1)
    {
        fprintf(stderr, "[TLS ERROR] Could not set minimum TLS version.\n");

        ERR_print_errors_fp(stderr);

        return -1;
    }

    /*
     * Disable TLS compression.
     */
    SSL_CTX_set_options(
        context,
        SSL_OP_NO_COMPRESSION
    );

    return 0;
}


/*
 * Create a TLS server context.
 */
SSL_CTX *tls_create_server_context(
    const char *certificate_file,
    const char *private_key_file
)
{
    if (certificate_file == NULL || private_key_file == NULL)
    {
        fprintf(stderr, "[TLS ERROR] Invalid server certificate paths.\n");

        return NULL;
    }

    SSL_CTX *context = SSL_CTX_new(TLS_server_method());

    if (context == NULL)
    {
        fprintf(stderr, "[TLS ERROR] Could not create server context.\n");

        ERR_print_errors_fp(stderr);

        return NULL;
    }

    if (tls_configure_context(context) != 0)
    {
        SSL_CTX_free(context);

        return NULL;
    }

    /*
     * Load the server certificate.
     */
    if (SSL_CTX_use_certificate_chain_file(
            context,
            certificate_file
        ) != 1)
    {
        fprintf(stderr, "[TLS ERROR] Could not load server certificate.\n");

        ERR_print_errors_fp(stderr);

        SSL_CTX_free(context);

        return NULL;
    }

    /*
     * Load the server private key.
     */
    if (SSL_CTX_use_PrivateKey_file(
            context,
            private_key_file,
            SSL_FILETYPE_PEM
        ) != 1)
    {
        fprintf(stderr, "[TLS ERROR] Could not load server private key.\n");

        ERR_print_errors_fp(stderr);

        SSL_CTX_free(context);

        return NULL;
    }

    /*
     * Verify that the private key matches the certificate.
     */
    if (SSL_CTX_check_private_key(context) != 1)
    {
        fprintf(stderr, "[TLS ERROR] Server private key does not match certificate.\n");

        ERR_print_errors_fp(stderr);

        SSL_CTX_free(context);

        return NULL;
    }

    printf("[TLS] Server context created successfully.\n");

    return context;
}


/*
 * Create a TLS client context.
 */
SSL_CTX *tls_create_client_context(
    const char *trusted_certificate_file
)
{
    if (trusted_certificate_file == NULL)
    {
        fprintf(stderr, "[TLS ERROR] Invalid trusted certificate path.\n");

        return NULL;
    }

    SSL_CTX *context = SSL_CTX_new(TLS_client_method());

    if (context == NULL)
    {
        fprintf(stderr, "[TLS ERROR] Could not create client context.\n");

        ERR_print_errors_fp(stderr);

        return NULL;
    }

    if (tls_configure_context(context) != 0)
    {
        SSL_CTX_free(context);

        return NULL;
    }

    /*
     * Require the server to present a trusted certificate.
     */
    SSL_CTX_set_verify(
        context,
        SSL_VERIFY_PEER,
        NULL
    );

    /*
     * Load the trusted server certificate.
     */
    if (SSL_CTX_load_verify_locations(
            context,
            trusted_certificate_file,
            NULL
        ) != 1)
    {
        fprintf(stderr, "[TLS ERROR] Could not load trusted certificate.\n");

        ERR_print_errors_fp(stderr);

        SSL_CTX_free(context);

        return NULL;
    }

    printf("[TLS] Client context created successfully.\n");

    return context;
}


/*
 * Release an SSL context.
 */
void tls_destroy_context(SSL_CTX *context)
{
    if (context != NULL)
    {
        SSL_CTX_free(context);
    }
}