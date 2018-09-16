#include "TcpClient.h"

#include "custom_lib.h"
#include "config.h"

void TcpClient_GlobalInit()
{
    Socket_GlobalInit();
}

void TcpClient_GlobalDeInit()
{
    Socket_GlobalDeInit();
}

void TcpClient_Init(TcpClient_t* h)
{
    if (h)
	{
		ZeroMemory_custom(h, sizeof(TcpClient_t));
		//TODO: other required init
	}
}

bool TcpClient_Start(TcpClient_t* h, sock_type_t sock_type, char* server_name, uint16_t port)
{
	if (!h) return false;
	if (sock_type == sock_none) return false;

    struct addrinfo hints;
    struct addrinfo *result, *ptr = nullptr;
    char port_buff[PORT_STRING_SIZE] = "";
    custom_sock_t connect_socket = CUSTOM_SOCK_INVALID;

    ZeroMemory_custom( &hints, sizeof(hints) );
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

    sprintf_custom(port_buff, "%u", port);

    // Resolve the server address and port
    int res = getaddrinfo(server_name, port_buff, &hints, &result);
    if ( res != 0 ) {
        fprintf(stderr, "getaddrinfo failed with error: %d\n", res);
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != nullptr ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connect_socket == INVALID_SOCKET) {
            fprintf(stderr, "socket failed with error: %ld\n", CUSTOM_SOCK_ERROR_CODE);
            return false;
        }

        // Connect to server.
        res = connect( connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (res == SOCKET_ERROR) {
            closesocket(connect_socket);
            connect_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (connect_socket == INVALID_SOCKET) {
        fprintf(stderr, "Unable to connect to server!\n");
        return false;
    }

	return true;
}

void TcpClient_Stop(TcpClient_t* h)
{
	if (h)
	{
        int res = shutdown(h->sock, SD_BOTH);
        if (res == SOCKET_ERROR) {
            fprintf(stderr, "shutdown failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
        }

		// No longer need socket
	    closesocket(h->sock);
	}
}
