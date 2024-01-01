#ifndef SOURCECODEREADER_H
#define SOURCECODEREADER_H

#include <fstream>
#include <sstream>
#include <string>
#include <stack>

class SourceCodeReader
{
public:
    enum class SourceType
    {
        SOURCE_NONE,
        SOURCE_FILE,
        SOURCE_MACRO
    };

    class SourceEntry
    {
    private:

    public:
        SourceType Type;
        std::string Name;
        std::istream* Stream;
        int LineNumber;

        SourceEntry(const std::string& Name);                           // For the top level File Stream
        SourceEntry(const std::string& Name, const std::string& Data);  // For Macro Expansions
    };

private:
    std::stack<SourceEntry> SourceStreams;
    const std::string Empty = "";

public:
    SourceCodeReader(const std::string& FileName);
    void InsertMacro(const std::string& Name, const std::string& Data);
    bool getLine(std::string& line);
    inline bool InMacro() const
    {
        return SourceStreams.size() > 0 ? SourceStreams.top().Type == SourceType::SOURCE_MACRO : false;
    };
    inline const std::string& StreamName() const
    {
        return SourceStreams.size() > 0 ? SourceStreams.top().Name : Empty;
    }
    inline const int LineNumber() const
    {
        return SourceStreams.top().LineNumber;
    }
};

#endif // SOURCECODEREADER_H
