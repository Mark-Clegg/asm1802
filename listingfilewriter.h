#ifndef LISTINGFILEWRITER_H
#define LISTINGFILEWRITER_H

#include <cstdint>
#include <vector>
#include "exceptions.h"
#include "sourcecodereader.h"

class ListingFileWriter
{
private:
    const SourceCodeReader& Source;
    std::string ListFileName;
    std::ofstream ListStream;

public:
    ListingFileWriter(const SourceCodeReader& Source, bool Enabled);
    ~ListingFileWriter();

    bool Enabled;

    void Append();
    void Append(const std::uint16_t Address, const std::vector<std::uint8_t>& Data);
    void AppendError(const std::string& Message, const AssemblyErrorSeverity Severity);
};

#endif // LISTINGFILEWRITER_H
