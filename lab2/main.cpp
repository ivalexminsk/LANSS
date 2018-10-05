#include "custom_sock.h"

#include <stdlib.h>
#include <stdio.h>
#include "TcpServ.h"
#include "sock_thread_handler.h"
#include "config.h"

int main() 
{
	TcpServ_GlobalInit();

	TcpServ_t serv;
	TcpServ_Init(&serv);
	if (TcpServ_Start(&serv, sock_tcp, COMMUNICATION_PORT))
	{
		while (true)
		{
			TcpServ_WaitConnection(&serv, sock_thread_callback);
		}
		TcpServ_Stop(&serv);
	}

	TcpServ_GlobalDeInit();

    return 0;
}
