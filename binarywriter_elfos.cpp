#include "binarywriter_elfos.h"

BinaryWriter_ElfOS::BinaryWriter_ElfOS(const std::string& FileName, const std::string& Extension) : BinaryWriter(FileName, Extension)
{
}

void BinaryWriter_ElfOS::Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress)
{
    std::optional<uint16_t> FirstBlock;

    uint16_t LoadAddress = 0xFFFF;
    uint16_t EndAddress = 0;
    uint16_t ExecAddress = 0;
    uint16_t Size;

    if(StartAddress.has_value())
        ExecAddress = StartAddress.value();

    for(const auto& Blob : Code)
        if(Blob.second.size() > 0)
        {
            if(Blob.first < LoadAddress)
                LoadAddress = Blob.first;
            if(Blob.first + Blob.second.size() > EndAddress)
                EndAddress = Blob.first + Blob.second.size();
        }
    Size = EndAddress - LoadAddress;

    // Generate ElfOS header
    std::vector<uint8_t> Header;
    Header.push_back(LoadAddress >> 8 & 0xff);
    Header.push_back(LoadAddress & 0xff);
    Header.push_back(Size >> 8 & 0xff);
    Header.push_back(Size & 0xff);
    Header.push_back(ExecAddress >> 8 & 0xff);
    Header.push_back(ExecAddress & 0xff);
    Output.write((const char *)&Header[0], 6);

    // Write binary data
    for(const auto& Blob : Code)
    {
        const std::vector<uint8_t>& DataIn = Blob.second;
        if(DataIn.size() > 0)
        {
            if(!FirstBlock.has_value())   // First Blob will have the lowest address, so should be used as the offset for all following blocks
                FirstBlock = Blob.first;
            else
            {
                int PadBytes = Blob.first - FirstBlock.value() - Output.tellp() + Header.size();
                for(int i = 0; i < PadBytes; i++)
                    Output.write("\0",1);
            }
            Output.write((const char *)&DataIn[0], DataIn.size());
        }
    }
}
