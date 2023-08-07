#include <filesystem>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include "listingfilewriter.h"

namespace fs = std::filesystem;

ListingFileWriter::ListingFileWriter(const SourceCodeReader& Src, bool Enabled) :
    Source { Src },
    Enabled { Enabled }
{
    auto p = fs::path(Source.getFileName());
    p.replace_extension("lst");
    ListFileName = p;
}

ListingFileWriter::~ListingFileWriter()
{
    if(ListStream.is_open())
        ListStream.close();
}

void ListingFileWriter::Append()
{
    if(Enabled)
    {
        if(!ListStream.is_open())
        {
            ListStream.open(ListFileName, std::ofstream::out | std::ofstream::trunc);
        }
        std::string FileName = fs::path(Source.getFileName()).filename();
        if(FileName.length() > 20)
            FileName = FileName.substr(0, 17) + "...";

        fmt::print(ListStream, "[{filename:22}({linenumber:5})] |                       {line}\n",
                   fmt::arg("filename", FileName),
                   fmt::arg("linenumber", Source.getLineNumber()),
                   fmt::arg("line", Source.getLastLine())
                   );
    }
}

void ListingFileWriter::Append(const std::uint16_t Address, const std::vector<std::uint8_t>& Data)
{
    if(Enabled)
    {
        if(!ListStream.is_open())
        {
            ListStream.open(ListFileName, std::ofstream::out | std::ofstream::trunc);
        }
        std::string FileName = fs::path(Source.getFileName()).filename();
        if(FileName.length() > 20)
            FileName = FileName.substr(0, 17) + "...";

        for(int i = 0; i < (Data.size() - 1) / 4 + 1; i++)
        {
            if(i == 0)
                fmt::print(ListStream, "[{filename:22}({linenumber:5})] | {address:04X}   ",
                           fmt::arg("filename", FileName),
                           fmt::arg("linenumber", Source.getLineNumber()),
                           fmt::arg("address", Address)
                );
            else
                fmt::print(ListStream, "{space:41}", fmt::arg("space", " "));

            for(int j = 0; j < 4; j++)
                if((i*4)+j < Data.size())
                    fmt::print(ListStream, "{byte:02X} ", fmt::arg("byte", Data[i*4+j]));
                else
                    fmt::print(ListStream, "{space:2} ", fmt::arg("space", ""));

            if(i == 0)
                fmt::print(ListStream, "|  {line}", // Initial spaces to pad line start to an 8 character boundary (to align tabs)
                           fmt::arg("line", Source.getLastLine())
                );
            ListStream << std::endl;
        }
    }
}

void ListingFileWriter::AppendError(const std::string& Message, const AssemblyErrorSeverity Severity)
{
    if(Enabled)
    {
        if(!ListStream.is_open())
        {
            ListStream.open(ListFileName, std::ofstream::out | std::ofstream::trunc);
        }
        fmt::print(ListStream, "*******************************************{severity:*>10}:  {message}\n",
                   fmt::arg("severity", " "+AssemblyError::SeverityName.at(Severity)),
                   fmt::arg("message", Message));
    }
}
