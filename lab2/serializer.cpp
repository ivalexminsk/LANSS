#include "serializer.h"

#if 0
struct MyStruct {
    uint32_t i;
    char str[6];
    std::vector<float> fs;
};

template <typename S>
void serialize(S& s, MyStruct& o) {
    s.value4b(o.i);
    s.text1b(o.str);
    s.container4b(o.fs, 100);
}

void example()
{
    MyStruct data{8941, "hello", {15.0f, -8.5f, 0.045f}};
    MyStruct res{};

    Buffer buffer;
    auto writtenSize = quickSerialization<OutputAdapter>(buffer, data);

    auto state = quickDeserialization<InputAdapter>({buffer.begin(), writtenSize}, res);

    assert(state.first == ReaderError::NoError && state.second);
    assert(data.fs == res.fs && data.i == res.i && std::strcmp(data.str, res.str) == 0);
}

#endif

size_t payload_struct_serialize(send_recv_payload_t& info, Buffer& buff)
{
	buff.clear();
	return quickSerialization<OutputAdapter>(buff, info);
}

bool payload_struct_deserialize(send_recv_payload_t& info, Buffer& buff, size_t buff_size)
{
	const auto state = quickDeserialization<InputAdapter>({buff.begin(), buff_size}, info);
	return (state.first == ReaderError::NoError && state.second);
}
