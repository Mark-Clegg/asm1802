#ifndef SYMBOL_H
#define SYMBOL_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>

class symbolDefinition
{
public:
    symbolDefinition();

    std::optional<uint16_t> Address;
    bool Public = false;
    bool Extern = false;
};

class symbolTable : public std::map<std::string,symbolDefinition>
{
public:
    symbolTable();
    symbolTable(bool Relocatable);

    const bool Master;
    const bool Relocatable;
};

#endif // SYMBOL_H
