#include "assemblyexception.h"
#include <fmt/core.h>

const std::map<AssemblyErrorSeverity, std::string> AssemblyException::SeverityName =
{
    { SEVERITY_Warning, "Warning"     },
    { SEVERITY_Error,   "Error"       }
};

AssemblyException::AssemblyException(const std::string& Message, AssemblyErrorSeverity Severity, OpCodeTypeEnum OpCodeType)
{
    this->Message = Message;
    this->Severity = Severity;
    this->BytesToSkip = OpCodeTable::OpCodeBytes.at(OpCodeType);
}

AssemblyException::AssemblyException(const std::string& Message, AssemblyErrorSeverity Severity, OpCodeEnum SkipToOpCode)
{
    this->Message = Message;
    this->Severity = Severity;
    this->SkipToOpCode = SkipToOpCode;
}

const char* AssemblyException::what() const throw()
{
    return Message.c_str();
}
