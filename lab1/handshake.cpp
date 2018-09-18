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
    if (!other_answer)
    {
        return false;
    }

    ans = (int)send(s, (char*)(&my_answer), sizeof(my_answer), 0);
    if (ans != sizeof(my_answer))
    {
        return false;
    }

    return true;
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
    if (!other_answer)
    {
        return false;
    }

    int ans = (int)send(s, (char*)(&my_answer), sizeof(my_answer), 0);
    if (ans != sizeof(my_answer))
    {
        return false;
    }

    if (Socket_Recv(s, buff, sizeof(other_answer)) != sizeof(other_answer))
    {
        return false;
    }
    return true;
}
