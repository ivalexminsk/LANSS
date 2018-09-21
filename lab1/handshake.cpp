#include "handshake.h"

#include <string>

bool three_way_handshake_up(custom_sock_t s, bool my_answer)
{
    int ans = (int)send(s, (char*)(&my_answer), sizeof(my_answer), 0);
    if (ans != sizeof(my_answer))
    {
        return false;
    }

    std::string buff;
    bool other_answer;
    if (Socket_Recv(s, buff, sizeof(other_answer)) != sizeof(other_answer))
    {
        return false;
    }

    other_answer = *((bool*)buff.data());
    my_answer &= other_answer;

    ans = (int)send(s, (char*)(&my_answer), sizeof(my_answer), 0);
    if (ans != sizeof(my_answer))
    {
        return false;
    }

    return my_answer;
}

bool three_way_handshake_down(custom_sock_t s, bool my_answer)
{
    std::string buff;
    bool other_answer;
    if (Socket_Recv(s, buff, sizeof(other_answer)) != sizeof(other_answer))
    {
        return false;
    }

    other_answer = *((bool*)buff.data());
    my_answer &= other_answer;

    int ans = (int)send(s, (char*)(&my_answer), sizeof(my_answer), 0);
    if (ans != sizeof(my_answer))
    {
        return false;
    }

    if (Socket_Recv(s, buff, sizeof(other_answer)) != sizeof(other_answer))
    {
        return false;
    }
    return (my_answer && other_answer);
}

bool get_last_received(custom_sock_t s, uint64_t* answer)
{
    if (!answer) return false;

    std::string buff;
    if (Socket_Recv(s, buff, sizeof(*answer)) != sizeof(*answer))
    {
        return false;
    }

    *answer = *((uint64_t*)buff.data());
    return true;
}

bool send_last_received(custom_sock_t s, uint64_t answer)
{
    int ans = (int)send(s, (char*)(&answer), sizeof(answer), 0);
    return (ans != sizeof(answer));
}
