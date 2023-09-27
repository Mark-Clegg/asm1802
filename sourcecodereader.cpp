#include "assemblyexception.h"
#include "sourcecodereader.h"

SourceCodeReader::SourceEntry::SourceEntry(SourceType Type, const std::string& Name) :
    Type(Type),
    Name(Name),
    LineNumber(0)
{
    switch(Type)
    {
    case SOURCE_FILE:
        Stream = new std::ifstream(Name);
        if(Stream->fail())
            throw AssemblyException("Unable to open " + Name, SEVERITY_Error);
        break;
    case SOURCE_LITERAL:
        Stream = new std::istringstream(Name);
        break;
    }
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

const std::string& SourceCodeReader::getFileName() const
{
    static const std::string Empty = "";

    if(SourceStreams.size() > 0)
    {
        switch(SourceStreams.top().Type)
        {
        case SOURCE_FILE:
            return SourceStreams.top().Name;
            break;
        case SOURCE_LITERAL:
            return Empty;
            break;
        }
    }
    return Empty;
}

void SourceCodeReader::IncludeFile(const std::string& FileName)
{
    try {
        SourceEntry Entry(SOURCE_FILE, FileName);
        SourceStreams.push(Entry);
    } catch (...) {
        throw AssemblyException("Unable to open " + FileName, SEVERITY_Error);
    }
}

void SourceCodeReader::IncludeLiteral(const std::string& Data)
{
    SourceEntry Entry(SOURCE_LITERAL, Data);
    SourceStreams.push(Entry);
}
