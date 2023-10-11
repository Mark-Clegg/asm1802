#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "macro.h"

struct SymbolDefinition
{
    std::optional<uint16_t> Value;
    bool HideFromSymbolTable = false;
};

class SymbolTable
{
public:
    std::string Name;
    SymbolTable();

    int CodeSize = 0;;                               // Size of subroutine code, used when calculating auto alignment
    std::string EntryPointLabel;                     // Entry Point, if specifid by SUBROUTINE ENTRYPOINT = ... parameter
    std::map<std::string, SymbolDefinition> Symbols; // Symbol Table
    std::map<std::string, Macro> Macros;             // Macro Definitions
};

#endif // SYMBOLTABLE_H
