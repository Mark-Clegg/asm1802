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
