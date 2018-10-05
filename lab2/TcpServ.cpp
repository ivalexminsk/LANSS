#include "TcpServ.h"

#include <cstdio>
#include <string.h>
#include "config.h"
#include "custom_lib.h"

void TcpServ_GlobalInit()
{
	Socket_GlobalInit();
}

void TcpServ_GlobalDeInit()
{
	Socket_GlobalDeInit();
}

void TcpServ_Init(TcpServ_t* h)
{
	if (h)
	{
		ZeroMemory_custom(h, sizeof(TcpServ_t));
		//TODO: other required init
	}
}

bool TcpServ_Start(TcpServ_t* h, sock_type_t sock_type, uint16_t port)
{
	if (!h) return false;
	if (sock_type == sock_none) return false;

	h->serv_sock = Socket_Start(sock_type, nullptr, port, true);
	return (h->serv_sock != CUSTOM_SOCK_INVALID);
}

void TcpServ_WaitConnection(TcpServ_t* h, ready_message_callback_t callback)
{
	if (!h || !callback) return;

	printf("Client waiting...\n");
	custom_sock_t client_socket = accept(h->serv_sock, nullptr, nullptr);
    if (client_socket == CUSTOM_SOCK_INVALID) {
        fprintf(stderr, "accept failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
    }

	Socket_SetTimeouts(client_socket, true);

	printf("Client connected\n");

	// call user callback
	callback(client_socket);

	printf("Client disconnecting...\n");

	// shutdown the connection since we're done
    int res = shutdown(client_socket, SD_BOTH);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "shutdown failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
    }

	// cleanup
	closesocket(client_socket);

	printf("Client disconnected\n");
}

void TcpServ_Stop(TcpServ_t* h)
{
	if (h)
	{
		// No longer need server socket
	    closesocket(h->serv_sock);
	}
}
