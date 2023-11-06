#include "expressionexception.h"

ExpressionException::ExpressionException(const std::string& Message) :
    Message(Message)
{
}

const char* ExpressionException::what() const throw()
{
    return Message.c_str();
}
