#include "assemblyexception.h"
#include "sourcecodereader.h"

SourceCodeReader::SourceEntry::SourceEntry(const std::string& Name) :
    Name(Name),
    LineNumber(0)
{
    this->Type = SourceType::SOURCE_FILE;
    Stream = new std::ifstream(Name);
        if(Stream->fail())
            throw AssemblyException("Unable to open " + Name, SEVERITY_Error);
}

SourceCodeReader::SourceEntry::SourceEntry(const std::string& Name, const std::string& Data) :
    Name(Name),
    LineNumber(0)
{
    this->Type = SourceType::SOURCE_LITERAL;
    Stream = new std::istringstream(Data);
}

bool SourceCodeReader::getLine(std::string &Line)
{
    while(SourceStreams.size() > 0)
    {
        if(std::getline(*SourceStreams.top().Stream, Line))
        {
            // remove last character if \n or \r (convert MS-DOS line endings)
            if(Line.size() > 0 && (Line[Line.size()-1] == '\r' || Line[Line.size()-1] == '\n'))
                Line.pop_back();

            LastLine = Line;
            SourceStreams.top().LineNumber++;
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

const std::string& SourceCodeReader::getLastLine() const
{
    return LastLine;
}

const int SourceCodeReader::getLineNumber() const
{
    if(SourceStreams.size() > 0)
        return SourceStreams.top().LineNumber;
    else
        throw std::exception();
}

const std::string& SourceCodeReader::getName() const
{
    return SourceStreams.top().Name;
}

const SourceCodeReader::SourceType SourceCodeReader::getStreamType() const
{
    return SourceStreams.top().Type;
}

void SourceCodeReader::IncludeFile(const std::string& FileName)
{
    try {
        SourceEntry Entry(FileName);
        SourceStreams.push(Entry);
    } catch (...) {
        throw AssemblyException("Unable to open " + FileName, SEVERITY_Error);
    }
}

void SourceCodeReader::IncludeLiteral(const std::string& Name, const std::string& Data)
{
    SourceEntry Entry(Name, Data);
    SourceStreams.push(Entry);
}
