#include "assemblyexception.h"
#include "sourcecodereader.h"

SourceCodeReader::SourceEntry::SourceEntry(const std::string& Name) :
    Name(Name)
{
    this->Type = SourceType::SOURCE_FILE;
    Stream = new std::ifstream(Name);
    if(Stream->fail())
        throw AssemblyException("Unable to open " + Name, SEVERITY_Error);
}

SourceCodeReader::SourceEntry::SourceEntry(const std::string& Name, const std::string& Data) :
    Name(Name)
{
    this->Type = SourceType::SOURCE_MACRO;
    Stream = new std::istringstream(Data);
    LineNumber = 0;
}

SourceCodeReader::SourceCodeReader(const std::string& FileName)
{
    if(SourceStreams.size() > 100)
        throw AssemblyException("Source File Nesting limit exceeded", SEVERITY_Error);
    try
    {
        SourceEntry Entry(FileName);
        SourceStreams.push(Entry);
    }
    catch (...)
    {
        throw AssemblyException("Unable to open " + FileName, SEVERITY_Error);
    }
}

bool SourceCodeReader::getLine(std::string &Line)
{
    while(SourceStreams.size() > 0)
    {
        SourceStreams.top().LineNumber++;
        if(std::getline(*SourceStreams.top().Stream, Line))
        {
            // remove last character if \n or \r (convert MS-DOS line endings)
            if(Line.size() > 0 && (Line[Line.size()-1] == '\r' || Line[Line.size()-1] == '\n'))
                Line.pop_back();
            return true;
        }
        else
        {
            delete SourceStreams.top().Stream;
            SourceStreams.pop();
        }
    }
    return false;
}

void SourceCodeReader::InsertMacro(const std::string& Name, const std::string& Data)
{
    if(SourceStreams.size() > 16)
        throw AssemblyException("Maximum Macro nesting level exceeded", SEVERITY_Error);
    SourceEntry Entry(Name, Data);
    SourceStreams.push(Entry);
}
