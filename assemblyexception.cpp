#include "assemblyexception.h"
#include <fmt/core.h>

const std::map<AssemblyErrorSeverity, std::string> AssemblyException::SeverityName = {
    { SEVERITY_Warning, "Warning"     },
    { SEVERITY_Error,   "Error"       }
};

AssemblyException::AssemblyException(const std::string& Message, AssemblyErrorSeverity Severity, bool ShowLine)
{
    this->Message = Message;
    this->Severity = Severity;
    this->ShowLine = ShowLine;
}
