#pragma once

#ifdef _WIN32
/* Windows full support */

/* Headers */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

/* Libs */
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

/* Typedefs */
typedef SOCKET custom_sock_t;

/* Defines */
#define CUSTOM_SOCK_ERROR_CODE WSAGetLastError()
#define CUSTOM_SOCK_INVALID INVALID_SOCKET

#elif defined (__linux__) || defined(__unix__)

/* Headers */
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/* Typedefs */
typedef int custom_sock_t;

/* Defines */
#define CUSTOM_SOCK_ERROR_CODE errno
#define CUSTOM_SOCK_INVALID (-1)
#define SOCKET_ERROR (-1)

/* shutdown() ports */
#define SD_RECEIVE					SHUT_RD
#define SD_SEND						SHUT_WR
#define SD_BOTH						SHUT_RDWR

/* close socket fix */
#define closesocket close

#else

#error Platform is not supported. Edit "custom_sock.h" file for adding more supported platrofms

#endif

typedef enum sock_type_t
{
	sock_none = 0,		/* Default error value */
	sock_tcp,
	sock_udp,
} sock_type_t;

typedef void (*ready_message_callback_t) (custom_sock_t s);

void Socket_GlobalInit();
void Socket_GlobalDeInit();
