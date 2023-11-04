#ifndef PREPROCESSOREXPRESSIONEXCEPTION_H
#define PREPROCESSOREXPRESSIONEXCEPTION_H

#include <exception>
#include <string>

class PreProcessorExpressionException : public std::exception
{
public:
    PreProcessorExpressionException(const std::string& Message);
    const char* what() const throw();
private:
    const std::string Message = "";
};

#endif // PREPROCESSOREXPRESSIONEXCEPTION_H
