#ifndef BLOB_H
#define BLOB_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

class symbolDefinition
{
public:
    symbolDefinition();

    std::optional<uint16_t> Address;
    bool Public = false;
    bool Extern = false;
};

class blob
{
public:
    blob();
    blob(bool Relocatable);
    blob(bool Relocatable, uint16_t Address);

    const bool Master;
    const bool Relocatable;
    std::map<std::string,symbolDefinition> Symbols;
    std::map<uint16_t, std::vector<uint8_t>> Code;

    std::map<uint16_t, std::vector<uint8_t>>::iterator CurrentCode;
};

#endif // BLOB_H