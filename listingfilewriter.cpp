#include <filesystem>
#include <iomanip>
#include <iostream>
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

void ListingFileWriter::Append(/* uint8_t[] data */ )
{
    if(Enabled)
    {
        if(!ListStream.is_open())
        {
            ListStream.open(ListFileName, std::ofstream::out | std::ofstream::trunc);
        }
        std::string FileName = fs::path(Source.getFileName()).filename();
        ListStream << std::left << std::setw(10) << FileName << std::right << std::setw(5) << Source.getLineNumber() << ": " << Source.getLastLine() << std::endl;
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
        ListStream << "***************: " << AssemblyError::SeverityName.at(Severity) << ": " << Message << std::endl;
    }
}
