#include <optional>
#include "binarywriter_binary.h"

BinaryWriter_Binary::BinaryWriter_Binary(const std::string& FileName, const std::string& Extension) : BinaryWriter(FileName, Extension)
{
}

void BinaryWriter_Binary::Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress)
{
    std::optional<uint16_t> FirstBlock;

    for(const auto& Blob : Code)
    {
        const std::vector<uint8_t>& DataIn = Blob.second;
        if(DataIn.size() > 0)
        {
            int a_pos = Output.tellp();
            int a_first = Blob.first;

            if(!FirstBlock.has_value())   // First Blob will have the lowest address, so should be used as the offset for all following blocks
                FirstBlock = Blob.first;
            else
            {
                int PadBytes = Blob.first - FirstBlock.value() - Output.tellp();
                for(int i = 0; i < PadBytes; i++)
                    Output.write("\0",1);
            }
            Output.write((const char *)&DataIn[0], DataIn.size());
        }
    }
}
