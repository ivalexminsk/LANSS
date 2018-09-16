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

	struct addrinfo hints;
	struct addrinfo *result = nullptr;
	char port_buff[PORT_STRING_SIZE] = "";
	custom_sock_t listen_socket = CUSTOM_SOCK_INVALID;

	ZeroMemory_custom(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
	switch (sock_type)
	{
	case sock_tcp: 
		hints.ai_socktype = SOCK_STREAM;
	    hints.ai_protocol = IPPROTO_TCP;
		break;
	case sock_udp: 
		hints.ai_socktype = SOCK_DGRAM;
	    hints.ai_protocol = IPPROTO_UDP;
		break;
	case sock_none:
	default: 
		break;
	}
    hints.ai_flags = AI_PASSIVE;

	sprintf_custom(port_buff, "%u", port);

    // Resolve the server address and port
    int res = getaddrinfo(nullptr, port_buff, &hints, &result);
    if ( res != 0 ) {
        fprintf(stderr, "getaddrinfo failed with error: %d\n", res);
        return false;
    }

    // Create a SOCKET for connecting to server
    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket == CUSTOM_SOCK_INVALID) {
        fprintf(stderr, "socket failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
        freeaddrinfo(result);
        return false;
    }

    // Setup the TCP listening socket
    res = bind( listen_socket, result->ai_addr, (socklen_t)result->ai_addrlen);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "bind failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
        freeaddrinfo(result);
        closesocket(listen_socket);
        return false;
    }

    freeaddrinfo(result);

    res = listen(listen_socket, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        fprintf(stderr, "listen failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
        closesocket(listen_socket);
        return false;
    }

	h->serv_sock = listen_socket;
	return true;
}

void TcpServ_WaitConnection(TcpServ_t* h, ready_message_callback_t callback)
{
	if (!h || !callback) return;

	printf("Client waiting...\n");
	custom_sock_t client_socket = accept(h->serv_sock, nullptr, nullptr);
    if (client_socket == CUSTOM_SOCK_INVALID) {
        fprintf(stderr, "accept failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
    }

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
