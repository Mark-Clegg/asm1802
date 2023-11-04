#include "preprocessorexception.h"

PreProcessorException::PreProcessorException(const std::string& FileName, const int& LineNumber, const std::string& Message) :
    FileName(FileName),
    LineNumber(LineNumber),
    Message(Message)
{
}

const char *PreProcessorException::what() const throw()
{
    return Message.c_str();
}

PreProcessorExpressionException::PreProcessorExpressionException(const std::string& Message) :
    Message(Message)
{
}

const char* PreProcessorExpressionException::what() const throw()
{
    return Message.c_str();
}
