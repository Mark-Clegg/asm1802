#ifndef SOURCECODEREADER_H
#define SOURCECODEREADER_H

#include <fstream>
#include <sstream>
#include <string>
#include <stack>

class SourceCodeReader
{
public:
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

        SourceEntry(const std::string& Name);                           // For a File Stream
        SourceEntry(const std::string& Name, const std::string& Data);  // For a Literal Stream
    };

private:
    std::stack<SourceEntry> SourceStreams;
    std::string LastLine;

public:
    void IncludeFile(const std::string& FileName);
    void IncludeLiteral(const std::string& Name, const std::string& Data);
    bool getLine(std::string& line);
    const std::string& getLastLine() const;
    const int getLineNumber() const;
    const std::string& getName() const;
    const SourceType getStreamType() const;
};

#endif // SOURCECODEREADER_H
