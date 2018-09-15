#pragma once

#include <vector>
#include <stdint.h>

#include "config.h"

typedef struct send_recv_payload_t
{
    uint64_t sector_num;
    uint64_t sector_amount;
    bool is_last;
    std::vector<uint8_t> payload;
} send_recv_payload_t;
