#ifndef TLS_CONTEXT_H
#define TLS_CONTEXT_H

#include <openssl/ssl.h>

/*
 * Initialize OpenSSL.
 */
int tls_initialize(void);

/*
 * Create a TLS server context.
 *
 * certificate_file: Path to the server certificate.
 * private_key_file: Path to the server private key.
 *
 * Returns:
 * SSL_CTX pointer on success.
 * NULL on failure.
 */
SSL_CTX *tls_create_server_context(
    const char *certificate_file,
    const char *private_key_file
);

/*
 * Create a TLS client context.
 *
 * trusted_certificate_file: Certificate used to verify the server.
 *
 * Returns:
 * SSL_CTX pointer on success.
 * NULL on failure.
 */
SSL_CTX *tls_create_client_context(
    const char *trusted_certificate_file
);

/*
 * Release an SSL context.
 */
void tls_destroy_context(SSL_CTX *context);

#endif