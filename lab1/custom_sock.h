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

/* Defines */
#define CUSTOM_SOCK_ERROR_CODE WSAGetLastError()
#define CUSTOM_SOCK_INVALID INVALID_SOCKET

#else

#error Platform is not supported. Edit "custom_sock.h" file for adding more supported platrofms

#endif

typedef enum sock_type_t
{
	sock_none = 0,		/* Default error value */
	sock_tcp,
	sock_udp,
} sock_type_t;
