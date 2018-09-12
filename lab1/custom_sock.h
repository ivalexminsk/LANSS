#pragma once

#ifdef _WIN32
/* Windows full support */

/* Headers */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

/* Libs */
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

/* Typedefs */
typedef SOCKET custom_sock_t;

#else

#error Platform is not supported. Edit "custom_sock.h" file for adding more support

#endif

extern const custom_sock_t custom_sock_invalid;
