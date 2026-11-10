#ifndef TCP_FILE_TRANSFER_H
#define TCP_FILE_TRANSFER_H

#include <winsock2.h>

int tcp_send_file(
    SOCKET socket,
    const char *filename
);

int tcp_receive_file(
    SOCKET socket,
    const char *output_filename
);

#endif