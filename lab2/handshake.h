#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "custom_sock.h"

bool three_way_handshake_up(custom_sock_t s, bool my_answer);
bool three_way_handshake_down(custom_sock_t s, bool my_answer);
bool get_last_received(custom_sock_t s, uint64_t* answer);
bool send_last_received(custom_sock_t s, uint64_t answer);
