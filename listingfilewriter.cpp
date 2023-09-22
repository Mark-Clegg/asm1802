#include <filesystem>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include "listingfilewriter.h"

namespace fs = std::filesystem;

ListingFileWriter::ListingFileWriter(const SourceCodeReader& Src, const std::string& FileName, ErrorTable& Errors, bool Enabled) :
    Source { Src },
    Errors { Errors },
    Enabled { Enabled }
{
    auto p = fs::path(FileName);
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

        fmt::print(ListStream, "[{filename:22}({linenumber:5})] |                    |  {line}\n",
                   fmt::arg("filename", FileName),
                   fmt::arg("linenumber", Source.getLineNumber()),
                   fmt::arg("line", Source.getLastLine())
                   );

        PrintError(FileName, Source.getLineNumber());
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
                fmt::print(ListStream, "{space:32}|{space:8}", fmt::arg("space", " "));

            for(int j = 0; j < 4; j++)
                if((i*4)+j < Data.size())
                    fmt::print(ListStream, "{byte:02X} ", fmt::arg("byte", Data[i*4+j]));
                else
                    fmt::print(ListStream, "{space:2} ", fmt::arg("space", ""));
            ListStream << "|";
            if(i == 0)
                fmt::print(ListStream, "  {line}", // Initial spaces to pad line start to an 8 character boundary (to align tabs)
                           fmt::arg("line", Source.getLastLine())
                );
            ListStream << std::endl;
        }

        PrintError(FileName, Source.getLineNumber());
    }
}

void ListingFileWriter::PrintError(const std::string& FileName, int LineNumber)
{
    if(Errors.count(FileName) != 0)
    {
        auto range = Errors[FileName].equal_range(LineNumber);
        for(auto it = range.first; it != range.second; it++)
        {
            auto MsgSevPair = it->second;
            std::string Message = MsgSevPair.first;
            AssemblyErrorSeverity Severity = MsgSevPair.second;
            fmt::print(ListStream, "**************************************{severity:*>15}:  {message}\n",
                       fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                       fmt::arg("message", Message));
        }
    }
}

void ListingFileWriter::AppendSymbols(const std::string& Name, const blob& Blob)
{
    if(Enabled)
    {
        fmt::print(ListStream, "\n");
        std::string Title;
        if(Blob.Relocatable)
            Title = Name + " (Relocatable)";
        else
            Title = Name;
        fmt::print(ListStream, "{Title:-^108}\n", fmt::arg("Title", Title));

        int c = 0;
        for(auto& Symbol : Blob.Symbols)
        {
            fmt::print(ListStream, "{Name:15} ", fmt::arg("Name", Symbol.first));
            if(Symbol.second.Extern)
                fmt::print(ListStream, "External");
            else
            {
                if(Symbol.second.Value.has_value())
                    fmt::print(ListStream, "{Address:04X} {Public:3}",
                               fmt::arg("Address", Symbol.second.Value.value()),
                               fmt::arg("Public", Symbol.second.Public ? "Pub" : "   "));
                else
                    fmt::print(ListStream, "---- {Public:3}", fmt::arg("Public", Symbol.second.Public ? "Pub" : "   "));
            }
            if(++c % 4 == 0)
                fmt::print(ListStream, "\n");
            else
                fmt::print(ListStream, "    ");
        }
        fmt::print(ListStream, "\n");
    }
}
