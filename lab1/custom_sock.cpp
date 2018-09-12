#include "custom_sock.h"

const custom_sock_t custom_sock_invalid =
#ifdef _WIN32
	INVALID_SOCKET
#endif
;
