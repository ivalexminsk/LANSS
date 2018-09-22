#include <stdlib.h>
#include <stdio.h>

#include "custom_sock.h"
#include "TcpClient.h"
#include "config.h"
#include "sock_thread_handler.h"

int main(int argc, char **argv) 
{
    char connect_string[] = "127.0.0.1";
    TcpClient_t client;
    TcpClient_GlobalInit();

    TcpClient_Init(&client);

    do
        {
        bool is_started = TcpClient_Start(&client, sock_tcp, connect_string, COMMUNICATION_PORT);

        if (is_started)
        {
            TcpClient_Communicate(&client, sock_client_callback);
            TcpClient_Stop(&client);
        }
        else
        {
            fprintf(stderr, "Cannot start client\n");
        }
    } while (is_restore_connection);
    
    TcpClient_GlobalDeInit();

    return 0;
}
