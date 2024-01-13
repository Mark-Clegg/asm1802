#include "binarywriter_binary.h"

BinaryWriter_Binary::BinaryWriter_Binary(const std::string& FileName, const std::string& Extension)
{
    this->FileName = FileName;
    this->Extension = Extension;
}

void BinaryWriter_Binary::Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress)
{
    // Data Records

    for(const auto& Blob : Code)
    {
        uint16_t Address = Blob.first;
        const std::vector<uint8_t>& DataIn = Blob.second;
        if(DataIn.size() > 0)
        {
            auto p = fs::path(FileName);
            std::string newExtension = fmt::format("{addr:04X}.{ext}", fmt::arg("addr", Address), fmt::arg("ext", Extension));
            p.replace_extension(newExtension);

            fmt::println("Writing binary file: {FileName}...", fmt::arg("FileName", p.string()));
            Output.open(p, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

            Output.write((const char *)&DataIn[0], DataIn.size());
            Output.close();
        }
    }
}
