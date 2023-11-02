#include "errortable.h"

ErrorTable::ErrorTable()
{

}

//!
//! \brief PushError
//! \param SourceFileName
//! \param LineNumber
//! \param Line
//! \param Message
//! \param Severity
//!
//! Log the error for output during listing, omitting duplicates
//!
void ErrorTable::Push(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity, bool InMacro)
{
    std::string FileRef;
    std::pair<int,int> LineRef;
    if(InMacro)
    {
        FileRef = FileName+"::"+MacroName;
        LineRef = { LineNumber, MacroLineNumber };
    }
    else
    {
        FileRef = FileName;
        LineRef = { LineNumber, 0 };
    }

    if(Table.count(FileRef) == 0)
        Table.insert({FileRef, {}});

    if(Table[FileRef].count(LineRef) > 0)
    {
        auto range = Table[FileRef].equal_range(LineRef);
        bool match = false;
        for(auto it = range.first; it != range.second; it++)
        {
            auto MsgSevPair = it->second;
            match = (MsgSevPair.first == Message) && (MsgSevPair.second == Severity);
        }
        if(!match)
            Table[FileRef].insert({ LineRef, { Message, Severity}});
    }
    else
        Table[FileRef].insert({ LineRef, { Message, Severity}});
}

//!
//! \brief PushError
//! \param Message
//! \param Severity
//!
//! Log the error for output during listing, omitting duplicates
//!
void ErrorTable::Push(const std::string& Message, AssemblyErrorSeverity Severity)
{
    if(Table.count("") == 0)
        Table.insert({"", {}});

    Table[""].insert({ {0, 0}, { Message, Severity}});
}

//!
//! \brief ErrorTable::count
//! \param FileName
//! \return
//!
//! Return the number of errors for the given FileName
//!
int ErrorTable::count(const std::string& FileName)
{
    return Table.count(FileName);
}

//!
//! \brief count
//! \return
//!
//! Return the number of errors for the given severity
//!
int ErrorTable::count(const AssemblyErrorSeverity Severity)
{
    int Result = 0;
    for(auto& FileTable : Table)
        for(auto& LineTable : FileTable.second)
        {
            if(LineTable.second.second == Severity)
                Result++;
        }
    return Result;
}

//!
//! \brief Contains
//! \param FileName
//! \param LineNumber
//! \param Message
//! \param Severity
//! \return
//!
//! Check if the error table already contains the specified error
//!
bool ErrorTable::Contains(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Message, AssemblyErrorSeverity Severity, bool InMacro)
{
    std::string FileRef;
    std::pair<int,int> LineRef;
    if(InMacro)
    {
        FileRef = FileName+"::"+MacroName;
        LineRef = { LineNumber, MacroLineNumber };
    }
    else
    {
        FileRef = FileName;
        LineRef = { LineNumber, 0 };
    }

    auto FileTable = Table.find(FileRef);
    if(FileTable == Table.end())
        return false;

    auto& LineTable = FileTable->second;
    auto ErrorTable = LineTable.find(LineRef);
    if(ErrorTable == LineTable.end())
        return false;

    auto& MessagePair = ErrorTable->second;
    if(MessagePair.first == Message && MessagePair.second == Severity)
        return true;

    return false;
}

//!
//! \brief ErrorTable::operator []
//! \param FileName
//! \return
//!
//! Convenience operator[]
//!
std::multimap<std::pair<int,int>, std::pair<std::string, AssemblyErrorSeverity>>& ErrorTable::operator[](const std::string FileName)
{
    return Table[FileName];
}
