#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

int tcp_client_run(
    const char *server_ip,
    unsigned short port,
    const char *input_prefix
);

#endif