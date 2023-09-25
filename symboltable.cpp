#include "symboltable.h"

SymbolTable::SymbolTable() : Master(true), Relocatable(false)
{
}

SymbolTable::SymbolTable(bool Relocatable) : Master(false), Relocatable(Relocatable)
{
}

SymbolTable::SymbolTable(bool Relocatable, uint16_t Address) : Master(false), Relocatable(Relocatable)
{
}
