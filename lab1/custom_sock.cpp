#include "custom_sock.h"

#include <stdio.h>

void Socket_GlobalInit()
{
#ifdef _WIN32
	WSADATA wsa_data;

	const int res = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (res != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", res);
    }
#endif
}

void Socket_GlobalDeInit()
{
#ifdef _WIN32
	WSACleanup();
#endif
}
