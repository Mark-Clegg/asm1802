#ifndef EXPRESSIONTOKENIZER_H
#define EXPRESSIONTOKENIZER_H

#include <string>
#include <sstream>

enum TokenEnum
{
    TOKEN_CLOSE_BRACE,
    TOKEN_OPEN_BRACE,
    TOKEN_LABEL,
    TOKEN_NUMBER,
    TOKEN_DOT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_REMAINDER,
    TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT,
    TOKEN_BITWISE_AND,
    TOKEN_BITWISE_OR,
    TOKEN_BITWISE_XOR,
    TOKEN_BITWISE_NOT,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_LOGICAL_NOT,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_OR_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_OR_EQUAL,
    TOKEN_COMMA,
    TOKEN_END
};

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
