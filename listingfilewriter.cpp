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
    if(fs::exists(p))
        fs::remove(p);
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
        std::string FileName;
        std::string ErrorFileName;
        switch(Source.getStreamType())
        {
            case SourceCodeReader::SourceType::SOURCE_FILE:
                FileName = fs::path(Source.getName()).filename();
                ErrorFileName = FileName;
                if(FileName.length() > 20)
                    FileName = FileName.substr(0, 17) + "...";
                break;
            case SourceCodeReader::SourceType::SOURCE_LITERAL:
                FileName = fmt::format("Macro: {Name}", fmt::arg("Name", Source.getName()));
                ErrorFileName = Source.getName();
                break;
        }

        fmt::print(ListStream, "[{filename:22}({linenumber:5})] |                    |  {line}\n",
                   fmt::arg("filename", FileName),
                   fmt::arg("linenumber", Source.getLineNumber()),
                   fmt::arg("line", Source.getLastLine())
                  );

        PrintError(ErrorFileName, Source.getLineNumber());
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
        std::string FileName;
        std::string ErrorFileName;
        switch(Source.getStreamType())
        {
            case SourceCodeReader::SourceType::SOURCE_FILE:
                FileName = fs::path(Source.getName()).filename();
                ErrorFileName = FileName;
                if(FileName.length() > 20)
                    FileName = FileName.substr(0, 17) + "...";
                break;
            case SourceCodeReader::SourceType::SOURCE_LITERAL:
                FileName = fmt::format("Macro: {Name}", fmt::arg("Name", Source.getName()));
                ErrorFileName = Source.getName();
                break;
        }

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

        PrintError(ErrorFileName, Source.getLineNumber());
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

void ListingFileWriter::AppendGlobalErrors()
{
    if(Enabled)
    {
        if(Errors.count("") != 0)
        {
            auto range = Errors[""].equal_range(0);
            for(auto& it = range.first; it != range.second; it++)
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
}

void ListingFileWriter::AppendSymbols(const std::string& Name, const SymbolTable& Blob)
{
    if(Enabled)
    {
        fmt::print(ListStream, "{Name:-^116}\n", fmt::arg("Name", Name));

        int c = 0;
        for(auto& Symbol : Blob.Symbols)
            if(!Symbol.second.HideFromSymbolTable)
            {
                fmt::print(ListStream, "{Name:15} ", fmt::arg("Name", Symbol.first));
                if(Symbol.second.Value.has_value())
                    fmt::print(ListStream, "{Address:04X}", fmt::arg("Address", Symbol.second.Value.value()));
                else
                    fmt::print(ListStream, "----");
                if(++c % 5 == 0)
                    fmt::print(ListStream, "\n");
                else
                    fmt::print(ListStream, "    ");
            }
        fmt::print(ListStream, "\n");
    }
}
