#ifndef PREPROCESSOREXCEPTION_H
#define PREPROCESSOREXCEPTION_H

#include <string>

class PreProcessorException
{
public:
    PreProcessorException(const std::string& FileName, const int& LineNumber, const std::string& Message);
    const std::string FileName = "";
    const int LineNumber = 0;
    const std::string Message = "";
};

#endif // PREPROCESSOREXCEPTION_H
