#ifndef PREPROCESSOREXPRESSIONTOKENIZER_H
#define PREPROCESSOREXPRESSIONTOKENIZER_H

#include <sstream>
#include <string>
#include "expressiontokens.h"

class PreProcessorExpressionTokenizer
{
public:
    PreProcessorExpressionTokenizer();
    void Initialize(std::string& Expression);
    TokenEnum Get();
    TokenEnum Peek();
    int IntegerValue;
    std::string StringValue;
private:
    std::istringstream InputStream;
};

#endif // PREPROCESSOREXPRESSIONTOKENIZER_H
