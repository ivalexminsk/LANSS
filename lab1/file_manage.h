#pragma once

#include <stdio.h>
#include <string>

#include "custom_sock.h"
#include "config.h"

std::string execute_upload(custom_sock_t s, std::string& params);
std::string execute_download(custom_sock_t s, std::string& params);
std::string execute_continue_upload(custom_sock_t s, std::string& params);
std::string execute_continue_download(custom_sock_t s, std::string& params);
std::string append_newline(std::string& s);
