#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct SymbolDefinition
{
    std::optional<uint16_t> Value;
    bool HideFromSymbolTable = false;
};

class SymbolTable
{
public:
    SymbolTable();

    int CodeSize = 0;;              // Size of subroutine code, used when calculating auto alignment
    std::map<std::string, SymbolDefinition> Symbols;
};

#endif // SYMBOLTABLE_H
