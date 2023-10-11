#ifndef MACRO_H
#define MACRO_H

#include <vector>
#include <string>

class Macro
{
public:
    Macro();
    std::string Expansion;
    std::vector<std::string> Arguments;
};

#endif // MACRO_H
