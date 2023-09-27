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
    SymbolTable(bool Relocatable);
    SymbolTable(bool Relocatable, uint16_t Address);

    const bool Master;              // True for Main code , False for Subroutine
    const bool Relocatable;         // Only applicable for Subroutines.
    int CodeSize = 0;;              // Size of relocatable subroutine code, used to calculate alignment
    std::map<std::string, SymbolDefinition> Symbols;
};

#endif // SYMBOLTABLE_H
