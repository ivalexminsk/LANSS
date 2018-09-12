#include "file_manage.h"

std::string execute_upload(custom_sock_t s, std::string& params)
{
	//TODO:
	std::string temp("TODO: upload");
	return append_newline(temp);
}

std::string execute_download(custom_sock_t s, std::string& params)
{
	//TODO:
	std::string temp("TODO: download");
	return append_newline(temp);
}

std::string append_newline(std::string& s)
{
	return (s + SOCKET_COMMAND_DELIMITER_0 + SOCKET_COMMAND_DELIMITER_1);
}

std::string execute_continue_upload(custom_sock_t s, std::string& params)
{
	//TODO:
	std::string temp("TODO: upload continue");
	return append_newline(temp);
}
std::string execute_continue_download(custom_sock_t s, std::string& params)
{
	//TODO:
	std::string temp("TODO: download continue");
	return append_newline(temp);
}
