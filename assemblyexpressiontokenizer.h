#ifndef ASSEMBLYEXPRESSIONTOKENIZER_H
#define ASSEMBLYEXPRESSIONTOKENIZER_H

#include <string>
#include <sstream>
#include "expressiontokens.h"

class AssemblyExpressionTokenizer
{
public:
    AssemblyExpressionTokenizer();
    void Initialize(const std::string& Expression);
    TokenEnum Peek();
    TokenEnum Get();

    TokenEnum ID;
    std::string StringValue;
    int IntegerValue;

private:
    std::istringstream InputStream;
};

#endif // ASSEMBLYEXPRESSIONTOKENIZER_H
