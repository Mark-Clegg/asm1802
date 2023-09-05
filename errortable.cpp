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
void ErrorTable::Push(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity)
{
    if(Table.count(FileName) == 0)
        Table.insert({FileName, {}});

    if(Table[FileName].count(LineNumber) > 0)
    {
        auto range = Table[FileName].equal_range(LineNumber);
        bool match = false;
        for(auto it = range.first; it != range.second; it++)
        {
            auto MsgSevPair = it->second;
            match = (MsgSevPair.first == Message) && (MsgSevPair.second == Severity);
        }
        if(!match)
            Table[FileName].insert({ LineNumber, { Message, Severity}});
    }
    else
        Table[FileName].insert({ LineNumber, { Message, Severity}});
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
//! \brief ErrorTable::operator []
//! \param FileName
//! \return
//!
//! Convenience operator[]
//!
std::multimap<int, std::pair<std::string, AssemblyErrorSeverity>>& ErrorTable::operator[](const std::string FileName)
{
    return Table[FileName];
}
