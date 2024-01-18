#ifndef BINARYWRITER_NONE_H
#define BINARYWRITER_NONE_H

#include "binarywriter.h"

class BinaryWriter_None : public BinaryWriter
{
public:
    BinaryWriter_None(const std::string& FileName, const std::string& Extension);
    void Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress);

};

#endif // BINARYWRITER_NONE_H
