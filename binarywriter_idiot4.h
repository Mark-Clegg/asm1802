#ifndef BINARYWRITER_IDIOT4_H
#define BINARYWRITER_IDIOT4_H

#include "binarywriter.h"

class BinaryWriter_Idiot4 : public BinaryWriter
{
public:
    BinaryWriter_Idiot4(const std::string& FileName, const std::string& Extension);
    void Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress);
};

#endif // BINARYWRITER_IDIOT4_H
