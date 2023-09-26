#ifndef ASSEMBLYEXCEPTION_H
#define ASSEMBLYEXCEPTION_H

#include <exception>
#include <map>
#include <optional>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

enum AssemblyErrorSeverity
{
    SEVERITY_Warning,   // Assembly can continue, and correct code will be generated
    SEVERITY_Error,     // Assembly can continue, but incorrect code will be generated
};

class AssemblyException : public std::exception
{
public:
    AssemblyException(const std::string& Message, AssemblyErrorSeverity Severity = SEVERITY_Warning, bool ShowLine = true);
    std::string Message;
    AssemblyErrorSeverity Severity;
    bool ShowLine;

    static const std::map<AssemblyErrorSeverity, std::string> SeverityName;
};

#endif // ASSEMBLYEXCEPTION_H
