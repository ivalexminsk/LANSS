#pragma once

#include "custom_sock.h"

#include <stdint.h>

struct TcpClient_t
{
	custom_sock_t sock;
};

void TcpClient_GlobalInit();
void TcpClient_GlobalDeInit();

void TcpClient_Init(TcpClient_t* h);
bool TcpClient_Start(TcpClient_t* h, sock_type_t sock_type, char* server_name, uint16_t port);
void TcpClient_Stop(TcpClient_t* h);
