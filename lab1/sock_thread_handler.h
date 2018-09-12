#pragma once

#include <string>

#include "custom_sock.h"

typedef enum server_command_t
{
    server_command_none = 0,
    server_command_echo,
    server_command_time,
    server_command_disconnect,
    server_command_upload,
    server_command_download,
    server_command_upload_continue,
    server_command_download_continue,
} server_command_t;

#define SERVER_COMMAND_FIRST server_command_none
#define SERVER_COMMAND_LAST server_command_download_continue

void sock_thread_callback(custom_sock_t s);

/**
 * @brief Parse command from string buffer & remove command if it was found
 * 
 * @param buff 
 * @return server_command_t 
 */
server_command_t parse_command(std::string& buff);
void execute_command(custom_sock_t s, server_command_t c, std::string& params);

std::string execute_echo(custom_sock_t s, std::string& params);
std::string execute_time(custom_sock_t s, std::string& params);
std::string execute_disconnect(custom_sock_t s, std::string& params);
