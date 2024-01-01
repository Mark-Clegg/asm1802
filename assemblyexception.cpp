#include "assemblyexception.h"
#include <fmt/core.h>

const std::map<AssemblyErrorSeverity, std::string> AssemblyException::SeverityName =
{
    { AssemblyErrorSeverity::SEVERITY_Warning, "Warning"     },
    { AssemblyErrorSeverity::SEVERITY_Error,   "Error"       }
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
