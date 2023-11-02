#ifndef LISTINGFILEWRITER_H
#define LISTINGFILEWRITER_H

#include <cstdint>
#include <fstream>
#include <map>
#include <vector>
#include "errortable.h"
#include "symboltable.h"

class ListingFileWriter
{
private:
    std::string ListFileName;
    std::ofstream ListStream;

    void PrintError(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const bool InMacro);

public:
    ListingFileWriter(const std::string& FileName, ErrorTable& Errors, bool Enabled);
    ~ListingFileWriter();

    bool Enabled;

    void Append(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const bool InMacro);
    void Append(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const bool InMacro, const std::uint16_t Address, const std::vector<std::uint8_t>& Data);
    void AppendGlobalErrors();
    void AppendSymbols(const std::string& Name, const SymbolTable& Symbols);
    ErrorTable& Errors;
};

#endif // LISTINGFILEWRITER_H
