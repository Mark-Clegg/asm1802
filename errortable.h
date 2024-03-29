#include <map>
#include <string>
#include "assemblyexception.h"

#ifndef ERRORTABLE_H
#define ERRORTABLE_H

class ErrorTable
{
public:
    ErrorTable();
    void Push(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity, bool InMacro);
    void Push(const std::string& Message, AssemblyErrorSeverity Severity);
    int count(const std::string& FileName);
    int count(const AssemblyErrorSeverity);
    bool Contains(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Message, AssemblyErrorSeverity Severity, bool InMacro);
    std::multimap<std::pair<int,int>, std::pair<std::string, AssemblyErrorSeverity>>& operator[](const std::string FileName);

private:
    // Error Cache:
    // FileName: LineNumber: Error Message / Severity
    //                       Error Message / Severity
    //           LineNumber: Error Message / Severity
    // FileName: LineNumber: Error Message / Severity
    std::map<std::string, std::multimap<std::pair<int,int>,std::pair<std::string, AssemblyErrorSeverity>>> Table;
};

#endif // ERRORTABLE_H
