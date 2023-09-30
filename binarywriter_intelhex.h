#ifndef BINARYWRITER_INTELHEX_H
#define BINARYWRITER_INTELHEX_H

#include "binarywriter.h"

class BinaryWriter_IntelHex : public BinaryWriter
{
public:
    BinaryWriter_IntelHex(const std::string& FileName, const std::string& Extension);
    void Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress);
private:
    void AddCheckSum(std::vector<uint8_t>& Data);
};

#endif // BINARYWRITER_INTELHEX_H
