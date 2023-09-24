#ifndef EXPRESSIONTOKENIZER_H
#define EXPRESSIONTOKENIZER_H

#include <string>
#include <sstream>

enum TokenEnum
{
    TOKEN_CLOSEBRACE,
    TOKEN_OPENBRACE,
    TOKEN_LABEL,
    TOKEN_NUMBER,
    TOKEN_DOT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_REMAINDER,
    TOKEN_SHIFTLEFT,
    TOKEN_SHIFTRIGHT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_XOR,
    TOKEN_END
};

class ExpressionTokenizer
{
public:
    ExpressionTokenizer();
    void Initialize(const std::string& Expression);
    TokenEnum PeekToken();
    TokenEnum GetToken();

    TokenEnum ID;
    std::string StringValue;
    int IntegerValue;

private:
    std::istringstream InputStream;
};

#endif // EXPRESSIONTOKENIZER_H
