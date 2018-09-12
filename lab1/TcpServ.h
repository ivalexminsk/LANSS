﻿#pragma once

#include "custom_sock.h"

#include <cstdint>

struct TcpServ_t
{
	custom_sock_t serv_sock;
};

typedef void (*ready_message_callback_t) (custom_sock_t s);

void TcpServ_GlobalInit();
void TcpServ_GlobalDeInit();

void TcpServ_Init(TcpServ_t* h);
void TcpServ_Start(TcpServ_t* h, uint16_t port);
void TcpServ_WaitConnection(TcpServ_t* h, ready_message_callback_t callback);
void TcpServ_Stop(TcpServ_t* h);
