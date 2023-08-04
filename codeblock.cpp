#include "codeblock.h"

codeblock::codeblock()
{

}

codeblock& codeblock::operator<<(const std::uint8_t byte)
{
    Code.push_back(byte);
    return *this;
}

int codeblock::size()
{
    return Code.size();
}
