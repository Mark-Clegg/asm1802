#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>


class SymbolTable
{
public:
    SymbolTable();
    SymbolTable(bool Relocatable);
    SymbolTable(bool Relocatable, uint16_t Address);

    const bool Master;              // True for Main code , False for Subroutine
    const bool Relocatable;         // Only applicable for Subroutines.
    int CodeSize = 0;;                  // Size of relocatable subroutine code, used for relocation alignment
    std::map<std::string, std::optional<uint16_t>> Symbols;
};

#endif // SYMBOLTABLE_H
