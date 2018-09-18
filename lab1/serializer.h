#pragma once

#include <vector>
#include <stdint.h>

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>

#include "config.h"

using namespace bitsery;

using Buffer = std::vector<uint8_t>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

using Buffer = std::vector<uint8_t>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

typedef struct send_recv_payload_t
{
    uint64_t sector_current_num;
    uint64_t sector_amount;
    bool is_last;
    std::vector<uint8_t> payload;
} send_recv_payload_t;

template <typename S>
void serialize(S& s, send_recv_payload_t& o) {
    s.value8b(o.sector_current_num);
    s.value8b(o.sector_amount);
    s.value1b(o.is_last);
    s.container1b(o.payload, SERIALIZER_MAX_PAYLOAD_SIZE);
}

size_t payload_struct_serialize(send_recv_payload_t& info, Buffer& buff);
bool payload_struct_deserialize(send_recv_payload_t& info, Buffer& buff, size_t buff_size);
