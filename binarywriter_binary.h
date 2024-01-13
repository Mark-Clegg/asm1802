#ifndef BINARYWRITER_BINARY_H
#define BINARYWRITER_BINARY_H

#include "binarywriter.h"

class BinaryWriter_Binary : public BinaryWriter
{
public:
    BinaryWriter_Binary(const std::string& FileName, const std::string& Extension);
    void Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress);
private:
    std::string Extension;
};

#endif // BINARYWRITER_BINARY_H
