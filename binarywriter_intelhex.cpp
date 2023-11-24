#include "binarywriter_intelhex.h"
#include <fmt/ranges.h>
BinaryWriter_IntelHex::BinaryWriter_IntelHex(const std::string& FileName, const std::string& Extension) : BinaryWriter(FileName, Extension)
{
}

void BinaryWriter_IntelHex::Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress)
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
                std::vector<uint8_t> DataBlock =
                {
                    0,                                              // Byte Count (to be replaced)
                    (uint8_t)(((Address + i * 16) & 0xFF00) >> 8),  // Address (Hi)
                    (uint8_t)((Address + i * 16) & 0xFF),           // Address (Lo)
                    0                                               // Record Type 0
                };

                for(int j = 0; j < 16 && i * 16 + j < DataIn.size(); j++)
                {
                    RecordSize ++;
                    DataBlock.push_back(DataIn[i * 16 + j]);
                }
                DataBlock[0] = DataBlock.size() - 4;
                AddCheckSum(DataBlock);
                if(DataBlock.size() > 5)
                    fmt::println(Output, ":{:02X}", fmt::join(DataBlock, ""));
            }
        }
    }

    // Start Address Records

    if(StartAddress.has_value())
    {
        std::vector<uint8_t> Type3Record =
        {
            4,                                               // Byte Count
            0,                                               // Address (Hi)
            0,                                               // Address (Lo)
            3,                                               // Record Type 3
            0,                                               // High Start Segment Address
            0,                                               // ..
            (uint8_t)((StartAddress.value() & 0xFF00) >> 8), // ..
            (uint8_t)(StartAddress.value() & 0xFF),          // Low Start Segment Address
        };
        AddCheckSum(Type3Record);
        fmt::println(Output, ":{:02X}", fmt::join(Type3Record, ""));

        std::vector<uint8_t> Type5Record =
        {
            4,                                               // Byte Count
            0,                                               // Address (Hi)
            0,                                               // Address (Lo)
            5,                                               // Record Type 5
            0,                                               // High Start Segment Address
            0,                                               // ..
            (uint8_t)((StartAddress.value() & 0xFF00) >> 8), // ..
            (uint8_t)(StartAddress.value() & 0xFF),          // Low Start Segment Address
        };
        AddCheckSum(Type5Record);
        fmt::println(Output, ":{:02X}", fmt::join(Type5Record, ""));
    }

    // End Record

    fmt::println(Output, ":00000001FF");
}

void BinaryWriter_IntelHex::AddCheckSum(std::vector<uint8_t>& Data)
{
    int CheckSum = 0;

    for(auto i : Data)
        CheckSum += i;
    CheckSum = (~CheckSum  + 1) & 0xFF;
    Data.push_back(CheckSum);
}
