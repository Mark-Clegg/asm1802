#ifndef LISTINGFILEWRITER_H
#define LISTINGFILEWRITER_H

#include <cstdint>
#include <map>
#include <vector>
#include "assemblyexception.h"
#include "errortable.h"
#include "sourcecodereader.h"

class ListingFileWriter
{
private:
    const SourceCodeReader& Source;
    std::string ListFileName;
    std::ofstream ListStream;

    void PrintError(const std::string& FileName, int LineNumber);

public:
    ListingFileWriter(const SourceCodeReader& Source, const std::string& FileName, ErrorTable& Errors, bool Enabled);
    ~ListingFileWriter();

    bool Enabled;

    void Append();
    void Append(const std::uint16_t Address, const std::vector<std::uint8_t>& Data);
//    void PushError(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity);
    ErrorTable& Errors;
};

#endif // LISTINGFILEWRITER_H
