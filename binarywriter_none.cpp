#include "binarywriter_none.h"

BinaryWriter_None::BinaryWriter_None(const std::string& FileName, const std::string& Extension)// : BinaryWriter(FileName, Extension)
{
}

void BinaryWriter_None::Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress)
{
}
