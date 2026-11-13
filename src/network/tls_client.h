#ifndef TLS_CLIENT_H
#define TLS_CLIENT_H

#include <winsock2.h>
#include <openssl/ssl.h>

/*
 * Establish a TLS connection over an existing TCP socket.
 *
 * context: Initialized TLS client context.
 * socket: Connected Winsock socket.
 * server_ip: Expected IPv4 address of the server.
 *
 * Returns:
 * SSL pointer on success.
 * NULL on failure.
 */
SSL *tls_client_connect(
    SSL_CTX *context,
    SOCKET socket,
    const char *server_ip
);

/*
 * Gracefully shut down a TLS connection and release resources.
 */
void tls_client_disconnect(SSL *ssl);

#endif