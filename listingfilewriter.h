#ifndef LISTINGFILEWRITER_H
#define LISTINGFILEWRITER_H

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
    void AppendError(const std::string& Message, const AssemblyErrorSeverity Severity);
};

#endif // LISTINGFILEWRITER_H
