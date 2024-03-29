#ifndef BINARYWRITER_ELFOS_H
#define BINARYWRITER_ELFOS_H

#include "binarywriter.h"

class BinaryWriter_ElfOS : public BinaryWriter
{
public:
    BinaryWriter_ElfOS(const std::string& FileName, const std::string& Extension);
    void Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress);
};

#endif // BINARYWRITER_ELFOS_H
