#include "custom_sock.h"

#include <stdlib.h>
#include <stdio.h>
#include "TcpServ.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27015

void my_callback (custom_sock_t s)
{
	int iResult;

    custom_sock_t ClientSocket = s;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

	    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);

        // Echo the buffer back to the sender
            iSendResult = send( ClientSocket, recvbuf, iResult, 0 );
            if (iSendResult == SOCKET_ERROR) {
                fprintf(stderr, "send failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
                return;
            }
            printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            fprintf(stderr, "recv failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            return;
        }

    } while (iResult > 0);

}

int main() 
{
	TcpServ_GlobalInit();

	TcpServ_t serv;
	TcpServ_Init(&serv);
	if (TcpServ_Start(&serv, sock_tcp, DEFAULT_PORT))
	{
		while (true)
		{
			TcpServ_WaitConnection(&serv, my_callback);
		}
		TcpServ_Stop(&serv);
	}

	TcpServ_GlobalDeInit();

    return 0;
}
