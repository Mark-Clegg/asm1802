#ifndef BINARYWRITER_H
#define BINARYWRITER_H

#include <cstdint>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <map>
#include <optional>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

//!
//! \brief The BinaryWriter class
//! Abstrat base class for an output file writer.
//! Inherit this, and implement void BinaryWriter_subcloass::Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress)
//! See BinaryWriter_IntelHex for example
class BinaryWriter
{
public:
    BinaryWriter(const std::string& FileName, const std::string& Extension);
    virtual ~BinaryWriter();
    virtual void Write(std::map<uint16_t, std::vector<uint8_t>>& Code, std::optional<uint16_t> StartAddress) = 0;

protected:
    std::string FileName;
    std::ofstream Output;
};

#endif // BINARYWRITER_H
