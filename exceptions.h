#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <map>
#include <optional>
#include <string>

enum AssemblyErrorSeverity
{
    SEVERITY_Warning,   // Assembly can continue, and correct code will be generated
    SEVERITY_Error,     // Assembly can continue, but incorrect code will be generated
    SEVERITY_Fatal      // Assembly cannot continue
};

class AssemblyError : public std::exception
{
public:
    AssemblyError(const std::string& Message, AssemblyErrorSeverity Severity, const std::optional<std::string>& Line, const std::optional<int>& LineNumber);
    AssemblyError(const std::string& Message, AssemblyErrorSeverity Severity = SEVERITY_Warning);
    std::optional<std::string> Line;
    std::optional<int> LineNumber;
    std::string Message;
    AssemblyErrorSeverity Severity;

    static const std::map<AssemblyErrorSeverity, std::string> SeverityName;

    static int ErrorCount;
    static int WarningCount;
    static int FatalCount;

    static void ResetErrorCounters();
};

#endif // EXCEPTIONS_H
