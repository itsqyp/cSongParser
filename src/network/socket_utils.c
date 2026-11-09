#include <stdio.h>
#include <winsock2.h>

#include "socket_utils.h"


int network_initialize(void)
{
    WSADATA wsa_data;

    // Initialize Winsock 2.2.
    int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsa_data
    );

    if (result != 0)
    {
        printf("[NETWORK ERROR] Winsock initialization failed.\n");
        printf("Error Code: %d\n", result);

        return -1;
    }

    // Verify the negotiated Winsock version.
    if (LOBYTE(wsa_data.wVersion) != 2 ||
        HIBYTE(wsa_data.wVersion) != 2)
    {
        printf("[NETWORK ERROR] Winsock 2.2 is unavailable.\n");

        WSACleanup();

        return -1;
    }

    printf("[NETWORK] Winsock 2.2 initialized successfully.\n");

    return 0;
}


void network_cleanup(void)
{
    WSACleanup();

    printf("[NETWORK] Winsock resources released.\n");
}