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

    h->sock = Socket_Start(sock_type, server_name, port, false);

	return (h->sock != CUSTOM_SOCK_INVALID);
}

void TcpClient_Communicate(TcpClient_t* h, ready_message_callback_t callback)
{
    if(h && h->sock != CUSTOM_SOCK_INVALID)
    {
        callback(h->sock);
    }
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
