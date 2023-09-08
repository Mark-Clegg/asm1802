#include "symbol.h"

symbolDefinition::symbolDefinition()
{
}

symbolTable::symbolTable() : Master(true), Relocatable(false)
{
}

symbolTable::symbolTable(bool Relocatable) : Master(false), Relocatable(Relocatable)
{
}
