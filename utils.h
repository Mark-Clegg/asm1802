#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include "macro.h"
#include "opcodetable.h"

std::string trim(const std::string &);
const std::optional<OpCodeSpec> ExpandTokens(const std::string& Line, std::string& Label, std::string& OpCode, std::vector<std::string>& Operands);
void StringListToVector(std::string& Input, std::vector<std::string>& Output, char Delimiter);
int AlignFromSize(int Size);
inline void ToUpper(std::string& In)
{
    std::transform(In.begin(), In.end(), In.begin(), ::toupper);
}
void StringToByteVector(const std::string& Operand, std::vector<uint8_t>& Data);
void ExpandMacro(const Macro& Definition, const std::vector<std::string>& Operands, std::string& Output);

#endif // UTILS_H
