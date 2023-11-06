#ifndef EXPRESSIONEXCEPTION_H
#define EXPRESSIONEXCEPTION_H

#include <exception>
#include <string>

class ExpressionException : public std::exception
{
public:
    ExpressionException(const std::string& Message);
    const char* what() const throw();
private:
    const std::string Message = "";
};

#endif // EXPRESSIONEXCEPTION_H
