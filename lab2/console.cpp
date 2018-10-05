#include "console.h"

#include "config.h"

std::string readline()
{
    std::string res;
    char c;
    do
    {
        c = getchar();
        res.push_back(c);
    } while (c != SOCKET_COMMAND_DELIMITER_0 && c != SOCKET_COMMAND_DELIMITER_1);
    return res;
}
