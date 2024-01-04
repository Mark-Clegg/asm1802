#ifndef EXPRESSIONTOKENIZER_H
#define EXPRESSIONTOKENIZER_H

#include <map>
#include <regex>
#include <sstream>
#include <string>

class ExpressionTokenizer
{
public:
    enum class TokenEnum
    {
        TOKEN_BITWISE_AND,
        TOKEN_BITWISE_NOT,
        TOKEN_BITWISE_OR,
        TOKEN_BITWISE_XOR,
        TOKEN_CLOSE_BRACE,
        TOKEN_COMMA,
        TOKEN_DIVIDE,
        TOKEN_DOLLAR,
        TOKEN_DOT,
        TOKEN_END,
        TOKEN_EQUAL,
        TOKEN_GREATER,
        TOKEN_GREATER_OR_EQUAL,
        TOKEN_LABEL,
        TOKEN_LESS,
        TOKEN_LESS_OR_EQUAL,
        TOKEN_LOGICAL_AND,
        TOKEN_LOGICAL_NOT,
        TOKEN_LOGICAL_OR,
        TOKEN_MINUS,
        TOKEN_MULTIPLY,
        TOKEN_NOT_EQUAL,
        TOKEN_NUMBER,
        TOKEN_OPEN_BRACE,
        TOKEN_PLUS,
        TOKEN_QUOTED_STRING,
        TOKEN_REMAINDER,
        TOKEN_SHIFT_LEFT,
        TOKEN_SHIFT_RIGHT
    };

    ExpressionTokenizer();
    void Initialize(std::string& Expression);
    TokenEnum Peek();
    TokenEnum Get();
    bool GetCustomToken(std::regex Pattern);
    long IntegerValue;
    std::string StringValue;
private:
    std::istringstream InputStream;
    bool PeekValid = false;
    TokenEnum LastPeek;
    std::string QuotedString();

};

#endif // EXPRESSIONTOKENIZER_H
