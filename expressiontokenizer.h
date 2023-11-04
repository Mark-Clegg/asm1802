#ifndef EXPRESSIONTOKENIZER_H
#define EXPRESSIONTOKENIZER_H

#include <string>
#include <sstream>
#include "expressiontokens.h"

class ExpressionTokenizer
{
public:
    ExpressionTokenizer();
    void Initialize(const std::string& Expression);
    TokenEnum Peek();
    TokenEnum Get();

    TokenEnum ID;
    std::string StringValue;
    int IntegerValue;

private:
    std::istringstream InputStream;
};

#endif // EXPRESSIONTOKENIZER_H
