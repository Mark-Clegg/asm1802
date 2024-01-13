#include "binarywriter.h"

BinaryWriter::BinaryWriter()
{
}

BinaryWriter::BinaryWriter(const std::string& FileName, const std::string& Extension)
{
    auto p = fs::path(FileName);
    p.replace_extension(Extension);
    this->FileName = p;
    Output.open(this->FileName, std::ofstream::out | std::ofstream::trunc);
    fmt::print("Writing binary file: {FileName}... ", fmt::arg("FileName", this->FileName));
}

BinaryWriter::~BinaryWriter()
{
    fmt::println("Done");
    if(Output.is_open())
        Output.close();
}
