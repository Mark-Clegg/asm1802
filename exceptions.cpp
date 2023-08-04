#include "exceptions.h"

const std::map<AssemblyErrorSeverity, std::string> AssemblyError::SeverityName = {
    { SEVERITY_Warning, "Warning"     },
    { SEVERITY_Error,   "Error"       },
    { SEVERITY_Fatal,   "Fatal Error" }
};

AssemblyError::AssemblyError(const std::string& Message, AssemblyErrorSeverity Severity, const std::optional<std::string>& Line, const std::optional<int>& LineNumber)
{
    this->Line = Line;
    this->LineNumber = LineNumber;
    this->Message = Message;
    this->Severity = Severity;
}

AssemblyError::AssemblyError(const std::string& Message, AssemblyErrorSeverity Severity)
{
    this->Message = Message;
    this->Severity = Severity;
}

int AssemblyError::ErrorCount = 0;
int AssemblyError::WarningCount = 0;
int AssemblyError::FatalCount = 0;

void AssemblyError::ResetErrorCounters()
{
    ErrorCount = 0;
    WarningCount = 0;
    FatalCount = 0;
}
