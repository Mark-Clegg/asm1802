#include "binarywriter_idiot4.h"

BinaryWriter_Idiot4::BinaryWriter_Idiot4(const std::string& FileName, const std::string& Extension) : BinaryWriter(FileName, Extension)
{
}

void BinaryWriter_Idiot4::Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress)
{
    // Data Records

    for(const auto& Blob : Code)
    {
        uint16_t Address = Blob.first;
        const std::vector<uint8_t>& DataIn = Blob.second;
        if(DataIn.size() > 0)
        {
            for(int i = 0; i < DataIn.size() / 16 + 1; i++)
            {
                int RecordSize = 0;
                std::vector<uint8_t> DataBlock;
                for(int j = 0; j < 16 && i * 16 + j < DataIn.size(); j++)
                {
                    RecordSize ++;
                    DataBlock.push_back(DataIn[i * 16 + j]);
                }
                fmt::print(Output, "!M{:04X} {:02X}\n", Address + i * 16, fmt::join(DataBlock, " "));
            }
        }
    }
}
