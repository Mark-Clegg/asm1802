#ifndef ASSEMBLYEXCEPTION_H
#define ASSEMBLYEXCEPTION_H

#include <exception>
#include <map>
#include <optional>
#include <string>
#include <filesystem>
#include "opcodetable.h"

namespace fs = std::filesystem;

enum AssemblyErrorSeverity
{
    SEVERITY_Warning,   // Assembly can continue, and correct code will be generated
    SEVERITY_Error,     // Assembly can continue, but incorrect code will be generated
};

class AssemblyException : public std::exception
{
public:
    AssemblyException(const std::string& Message, AssemblyErrorSeverity Severity);
    AssemblyException(const std::string& Message, AssemblyErrorSeverity Severity, OpCodeEnum SkipToOpCode);
    std::string Message;
    AssemblyErrorSeverity Severity;

    static const std::map<AssemblyErrorSeverity, std::string> SeverityName;
    std::optional<OpCodeEnum> SkipToOpCode;
};

#endif // ASSEMBLYEXCEPTION_H
