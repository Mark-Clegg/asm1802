#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <string>

std::string Trim(const std::string &);

inline void ToUpper(std::string& In)
{
    std::transform(In.begin(), In.end(), In.begin(), ::toupper);
}

#endif // UTILS_H
