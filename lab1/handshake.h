#pragma once

#include <stdbool.h>
#include "custom_sock.h"

bool three_way_handshake_up(custom_sock_t s, bool my_answer);
bool three_way_handshake_down(custom_sock_t s, bool my_answer);
