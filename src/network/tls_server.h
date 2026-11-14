#ifndef TLS_SERVER_H
#define TLS_SERVER_H

#include <winsock2.h>
#include <openssl/ssl.h>

/*
 * Establish a TLS connection over an already-accepted TCP socket.
 *
 * context: Initialized TLS server context.
 * client_socket: Accepted Winsock socket.
 *
 * Returns:
 * SSL pointer on success.
 * NULL on failure.
 */
SSL *tls_server_accept(
    SSL_CTX *context,
    SOCKET client_socket
);

/*
 * Shut down a TLS connection and release its resources.
 */
void tls_server_disconnect(SSL *ssl);

#endif