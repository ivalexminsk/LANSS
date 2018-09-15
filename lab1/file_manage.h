#pragma once

#include <stdio.h>
#include <string>
#include <stdint.h>

#include "custom_sock.h"
#include "config.h"
#include "serializer.h"

typedef enum file_operation_t
{
	file_operation_none = 0,
	file_operation_upload,
	file_operation_download
} file_operation_t;

typedef struct file_session_t
{
	FILE* handle;
	file_operation_t current_operation;
	uint64_t next_part_to_send_recv;
} file_session_t;

std::string execute_upload(custom_sock_t s, std::string& params);
std::string execute_download(custom_sock_t s, std::string& params);
std::string execute_continue_upload(custom_sock_t s, std::string& params);
std::string execute_continue_download(custom_sock_t s, std::string& params);
std::string append_newline(std::string& s);

bool file_read(file_session_t& session, send_recv_payload_t& to_send);
bool file_write(file_session_t& session, send_recv_payload_t& to_recv);
