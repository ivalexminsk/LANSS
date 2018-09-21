#include "custom_sock.h"

#include <stdio.h>

#include "custom_lib.h"
#include "config.h"

void Socket_GlobalInit()
{
#ifdef _WIN32
	WSADATA wsa_data;

	const int res = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (res != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", res);
    }
#endif
}

void Socket_GlobalDeInit()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

custom_sock_t Socket_Start(sock_type_t sock_type, char* server_name, uint16_t port, bool is_server_socket)
{
    if (sock_type == sock_none) return CUSTOM_SOCK_INVALID;

    struct addrinfo hints;
	struct addrinfo *result = nullptr;
	struct addrinfo *ptr = nullptr;
	char port_buff[PORT_STRING_SIZE] = "";
	custom_sock_t listen_socket = CUSTOM_SOCK_INVALID;

	ZeroMemory_custom(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
	switch (sock_type)
	{
	case sock_tcp: 
		hints.ai_socktype = SOCK_STREAM;
	    hints.ai_protocol = IPPROTO_TCP;
		break;
	case sock_udp: 
		hints.ai_socktype = SOCK_DGRAM;
	    hints.ai_protocol = IPPROTO_UDP;
		break;
	case sock_none:
	default: 
		break;
	}
    if (is_server_socket)
    {
        hints.ai_flags = AI_PASSIVE;
    }

	sprintf_custom(port_buff, "%u", port);

    // Resolve the server address and port
    int res = getaddrinfo(server_name, port_buff, &hints, &result);
    if ( res != 0 ) {
        fprintf(stderr, "getaddrinfo failed with error: %d\n", res);
        return CUSTOM_SOCK_INVALID;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != nullptr ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server/server socket
        listen_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (listen_socket == CUSTOM_SOCK_INVALID) {
            fprintf(stderr, "socket failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            return CUSTOM_SOCK_INVALID;
        }

        if (!is_server_socket)
        {
            // Connect to server.
            res = connect( listen_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (res == SOCKET_ERROR) {
                closesocket(listen_socket);
                listen_socket = CUSTOM_SOCK_INVALID;
                continue;
            }

            Socket_SetTimeouts(listen_socket, false);
        }
        break;
    }

    if (is_server_socket)
    {
        // Setup the TCP listening socket
        res = bind( listen_socket, result->ai_addr, (socklen_t)result->ai_addrlen);
        if (res == SOCKET_ERROR) {
            fprintf(stderr, "bind failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            freeaddrinfo(result);
            closesocket(listen_socket);
            return CUSTOM_SOCK_INVALID;
        }
    }

    freeaddrinfo(result);

    if (listen_socket == CUSTOM_SOCK_INVALID) {
        fprintf(stderr, "Unable to connect to server!\n");
        return CUSTOM_SOCK_INVALID;
    }

    if (is_server_socket)
    {
        res = listen(listen_socket, SOMAXCONN);
        if (res == SOCKET_ERROR) {
            fprintf(stderr, "listen failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            closesocket(listen_socket);
            return CUSTOM_SOCK_INVALID;
        }
    }

    return listen_socket;
}

size_t Socket_Recv(custom_sock_t s, std::string& buff, size_t count, bool* is_error)
{
    size_t was_read = 0;
    
    int result = 1;
    char recv_byte;

    if (is_error)
    {
        *is_error = false;
    }

	// Receive until the peer shuts down the connection
    while (result > 0 && count > 0)
    {
        result = recv(s, &recv_byte, sizeof(recv_byte), 0);
        if (result > 0) 
        {
            buff.push_back(recv_byte);
            was_read++;
            count--;
        }
        else if (result < 0)
        {
            if (is_error)
            {
                *is_error = true;
            }
            fprintf(stderr, "recv failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            return was_read;
        }
    }

    return was_read;
}

size_t Socket_RecvEndLine(custom_sock_t s, std::string& buff, char stop_symbol, bool* is_error)
{
    size_t was_read = 0;
    
    int result = 1;
    char recv_byte = stop_symbol + 1;

    if (is_error)
    {
        *is_error = false;
    }

	// Receive until the peer shuts down the connection
    while (result > 0 && recv_byte != stop_symbol)
    {
        result = recv(s, &recv_byte, sizeof(recv_byte), 0);
        if (result > 0) 
        {
            buff.push_back(recv_byte);
            was_read++;
        }
        else if (result < 0)
        {
            if (is_error)
            {
                *is_error = true;
            }
            fprintf(stderr, "recv failed with error: %d\n", CUSTOM_SOCK_ERROR_CODE);
            return was_read;
        }
    }

    return was_read;
}

void Socket_SetTimeouts(custom_sock_t s, bool is_skip_recv_timeout)
{
    if (!s) return;

#ifndef _WIN32
    struct timeval timeout;      
    timeout.tv_sec = COMMUNICATION_TIMEOUT_SECONDS;
    timeout.tv_usec = 0;
#else
    DWORD timeout = COMMUNICATION_TIMEOUT_SECONDS * 1000;
#endif

    int optval = 1;
 
    if (setsockopt (s, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) < 0)
    {
        fprintf(stderr, "setsockopt keepalive was failed. Error code %d\n", CUSTOM_SOCK_ERROR_CODE);
    }

    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        fprintf(stderr, "setsockopt recv was failed. Error code %d\n", CUSTOM_SOCK_ERROR_CODE);
    }

    if (setsockopt (s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        fprintf(stderr, "setsockopt send was failed. Error code %d\n", CUSTOM_SOCK_ERROR_CODE);
    }
}
