#include "preprocessorexpressionexception.h"

PreProcessorExpressionException::PreProcessorExpressionException(const std::string& Message) :
    Message(Message)
{
}

const char* PreProcessorExpressionException::what() const throw()
{
    return Message.c_str();
}
