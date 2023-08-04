#ifndef CODEBLOCK_H
#define CODEBLOCK_H

#include <cstdint>
#include <vector>

class codeblock
{
public:
    codeblock();

    std::vector<std::uint8_t> Code;
    codeblock& operator<<(const std::uint8_t byte);
    int size();
};

#endif // CODEBLOCK_H
