#ifndef PREPROCESSOREXCEPTION_H
#define PREPROCESSOREXCEPTION_H

#include <exception>
#include <string>

class PreProcessorException : public std::exception
{
public:
    PreProcessorException(const std::string& FileName, const int& LineNumber, const std::string& Message);
    const std::string FileName = "";
    const int LineNumber = 0;
    const char* what() const throw();
private:
    const std::string Message = "";
};

#endif // PREPROCESSOREXCEPTION_H
