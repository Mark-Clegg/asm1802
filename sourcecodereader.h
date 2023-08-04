#ifndef SOURCECODEREADER_H
#define SOURCECODEREADER_H

#include <fstream>
#include <sstream>
#include <string>
#include <stack>

class SourceCodeReader
{
    enum SourceType
    {
        SOURCE_FILE,
        SOURCE_LITERAL
    };

    class SourceEntry
    {
    private:

    public:
        SourceType Type;
        std::string Name;
        int LineNumber;
        std::istream* Stream;

        SourceEntry(SourceType Type, const std::string& Name);
    };

private:
    std::stack<SourceEntry> SourceStreams;
    std::string LastLine;

public:
    void IncludeFile(const std::string& FileName);
    void IncludeLiteral(const std::string& Data);
    bool getLine(std::string& line);
    const std::string& getLastLine() const;
    const int getLineNumber() const;
    const std::string& getFileName() const;
};

#endif // SOURCECODEREADER_H
