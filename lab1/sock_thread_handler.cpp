#include "sock_thread_handler.h"

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "config.h"
#include "file_manage.h"
#include "console.h"

const char* server_command_converter[] = 
{
    "",
    "ECHO",
    "TIME",
    "DISCONNECT",
    "UPLOAD",
    "DOWNLOAD",
    "UPLOAD_CONTINUE",
    "DOWNLOAD_CONTINUE",
};

const server_command_t spec_mode_commands[] = 
{
    server_command_upload,
    server_command_download,
    server_command_upload_continue,
    server_command_download_continue,
};

void sock_thread_callback(custom_sock_t s)
{
    size_t result;
    std::string recv_buff;

	// Receive until the peer shuts down the connection
    do {
        result = Socket_RecvEndLine(s, recv_buff, SOCKET_COMMAND_DELIMITER_1);
        trunk_endl(recv_buff);

        server_command_t command = parse_command(recv_buff);
        execute_command(s, command, recv_buff);

        recv_buff.clear();

        if (command == server_command_disconnect)
        {
            /* For disconnecting */
            result = 0;
        }
    } while (result > 0);
}

server_command_t parse_command(std::string& buff)
{
    size_t delim_pos = buff.find_first_of(SOCKET_WORD_DELIMITER);
    if (delim_pos == std::string::npos)
    {
        delim_pos = buff.length();
    }

    std::string comparable_string = buff.substr(0, delim_pos);

    for(server_command_t i = SERVER_COMMAND_FIRST; i <= SERVER_COMMAND_LAST; )
    {
        if (comparable_string == server_command_converter[i])
        {
            if (buff.length() > delim_pos)
            {
                delim_pos++;
            }
            buff = buff.substr(delim_pos);
            
            return i;
        }
		i = (server_command_t)(i + 1);
    }
    return server_command_none;
}

void execute_command(custom_sock_t s, server_command_t c, std::string& params)
{
    std::string res;
    switch(c)
    {
    case server_command_echo:
		res = execute_echo(s, params);
        break;
    case server_command_time:
		res = execute_time(s, params);
        break;
    case server_command_disconnect:
		res = execute_disconnect(s, params);
        break;
    case server_command_upload:
		res = execute_upload(s, params);
        break;
    case server_command_download:
		res = execute_download(s, params);
        break;
    case server_command_upload_continue:
        res = execute_continue_upload(s, params);
        break;
	case server_command_download_continue:
        res = execute_continue_download(s, params);
        break;
    case server_command_none:
	default:
        res = "Unknown command";
        res = append_newline(trunk_endl(res));
        break;
    }

    if (res.length())
    {
        // Echo the buffer back to the sender
#ifdef _WIN32
		const int len = (int)res.length();
#else
		const size_t len = res.length();
#endif

        int send_result = send( s, res.c_str(), len, 0 );
        if (send_result == SOCKET_ERROR) {
            fprintf(stderr, "send failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            return;
        }
    }
}

std::string& trunk_endl(std::string& s)
{
    while(s.length() 
        && (s.back() == SOCKET_COMMAND_DELIMITER_0 
        || s.back() == SOCKET_COMMAND_DELIMITER_1))
    {
        s.pop_back();
    }
    return s;
}

std::string execute_echo(custom_sock_t s, std::string& params)
{
	return append_newline(trunk_endl(params));
}

std::string execute_time(custom_sock_t s, std::string& params)
{
    // current date/time based on current system
    time_t result = time(nullptr);
	char* time_res = asctime(gmtime(&result));
	std::string time_string(time_res);
	return append_newline(trunk_endl(time_string));
}

std::string execute_disconnect(custom_sock_t s, std::string& params)
{
	//see sock_thread_callback implementation
	std::string res = "Bye";
	return append_newline(trunk_endl(res));
}

server_command_t sock_client_spec_mode_if_need(custom_sock_t s, std::string command_string)
{
    server_command_t command = parse_command(trunk_endl(command_string));
    bool is_command_spec = false;
    for (uint16_t i = 0; i < (sizeof(spec_mode_commands)/sizeof(spec_mode_commands[0])); i++)
    {
        if (command == spec_mode_commands[i])
        {
            is_command_spec = true;
            break;
        }
    }

    if (is_command_spec)
    {
        //TODO:
    }

    return command;
}

void sock_client_callback(custom_sock_t s)
{
    server_command_t c;
    bool is_error;
    do
    {
        std::string input = readline();

#ifdef _WIN32
		const int len = (int)input.length();
#else
		const size_t len = input.length();
#endif
        int send_result = send( s, input.c_str(), len, 0 );
        if (send_result == SOCKET_ERROR) {
            fprintf(stderr, "send failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            return;
        }

        c = sock_client_spec_mode_if_need(s, input);

        std::string received_info;
        Socket_RecvEndLine(s, received_info, SOCKET_COMMAND_DELIMITER_1, &is_error);

        printf("%s", received_info.c_str());
    } while (c != server_command_disconnect && !is_error);
        
}
