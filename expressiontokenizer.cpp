#include "expressiontokenizer.h"
#include "expressionexception.h"
#include "utils.h"

const std::map<ExpressionTokenizer::TokenEnum, std::string> ExpressionTokenizer::TokenNames =
{
    { ExpressionTokenizer::TOKEN_CLOSE_BRACE,      "TOKEN_CLOSE_BRACE"     },
    { ExpressionTokenizer::TOKEN_OPEN_BRACE,       "TOKEN_OPEN_BRACE"      },
    { ExpressionTokenizer::TOKEN_LABEL,            "TOKEN_LABEL"           },
    { ExpressionTokenizer::TOKEN_NUMBER,           "TOKEN_NUMBER"          },
    { ExpressionTokenizer::TOKEN_DOT,              "TOKEN_DOT"             },
    { ExpressionTokenizer::TOKEN_DOLLAR,           "TOKEN_DOLLAR"          },
    { ExpressionTokenizer::TOKEN_PLUS,             "TOKEN_PLUS"            },
    { ExpressionTokenizer::TOKEN_MINUS,            "TOKEN_MINUS"           },
    { ExpressionTokenizer::TOKEN_MULTIPLY,         "TOKEN_MULTIPLY"        },
    { ExpressionTokenizer::TOKEN_DIVIDE,           "TOKEN_DIVIDE"          },
    { ExpressionTokenizer::TOKEN_REMAINDER,        "TOKEN_REMAINDER"       },
    { ExpressionTokenizer::TOKEN_SHIFT_LEFT,       "TOKEN_SHIFT_LEFT"      },
    { ExpressionTokenizer::TOKEN_SHIFT_RIGHT,      "TOKEN_SHIFT_RIGHT"     },
    { ExpressionTokenizer::TOKEN_BITWISE_AND,      "TOKEN_BITWISE_AND"     },
    { ExpressionTokenizer::TOKEN_BITWISE_OR,       "TOKEN_BITWISE_OR"      },
    { ExpressionTokenizer::TOKEN_BITWISE_XOR,      "TOKEN_BITWISE_XOR"     },
    { ExpressionTokenizer::TOKEN_BITWISE_NOT,      "TOKEN_BITWISE_NOT"     },
    { ExpressionTokenizer::TOKEN_LOGICAL_AND,      "TOKEN_LOGICAL_AND"     },
    { ExpressionTokenizer::TOKEN_LOGICAL_OR,       "TOKEN_LOGICAL_OR"      },
    { ExpressionTokenizer::TOKEN_LOGICAL_NOT,      "TOKEN_LOGICAL_NOT"     },
    { ExpressionTokenizer::TOKEN_EQUAL,            "TOKEN_EQUAL"           },
    { ExpressionTokenizer::TOKEN_NOT_EQUAL,        "TOKEN_NOT_EQUAL"       },
    { ExpressionTokenizer::TOKEN_GREATER,          "TOKEN_GREATER"         },
    { ExpressionTokenizer::TOKEN_GREATER_OR_EQUAL, "TOKEN_GREATER_OR_EQUAL"},
    { ExpressionTokenizer::TOKEN_LESS,             "TOKEN_LESS"            },
    { ExpressionTokenizer::TOKEN_LESS_OR_EQUAL,    "TOKEN_LESS_OR_EQUAL"   },
    { ExpressionTokenizer::TOKEN_COMMA,            "TOKEN_COMMA"           },
    { ExpressionTokenizer::TOKEN_END,              "TOKEN_END"             }
};

ExpressionTokenizer::ExpressionTokenizer()
{
}

void ExpressionTokenizer::Initialize(std::string& Expression)
{
    InputStream.str(Expression);
    InputStream.seekg(0);
    InputStream.clear();
    PeekValid = false;
}

ExpressionTokenizer::TokenEnum ExpressionTokenizer::Peek()
{
    if(!PeekValid)
    {
        std::streampos SavedPos = InputStream.tellg();
        LastPeek = Get();
        PeekValid = true;
        InputStream.clear();
        InputStream.seekg(SavedPos);
    }
    return LastPeek;
}

ExpressionTokenizer::TokenEnum ExpressionTokenizer::Get()
{
    PeekValid = false;
    TokenEnum Result;
    while(!InputStream.eof() && !InputStream.fail() && isspace(InputStream.peek()))
        InputStream.ignore();

    char FirstChar = InputStream.get();
    if(InputStream.eof() || InputStream.fail())
        Result = TOKEN_END;
    else
    {
        switch(FirstChar)
        {
            case '(':
                Result = TOKEN_OPEN_BRACE;
                break;
            case ')':
                Result = TOKEN_CLOSE_BRACE;
                break;
            case '.':
                Result = TOKEN_DOT;
                break;
            case '+':
                Result = TOKEN_PLUS;
                break;
            case '-':
                Result = TOKEN_MINUS;
                break;
            case '*':
                Result = TOKEN_MULTIPLY;
                break;
            case '/':
                Result = TOKEN_DIVIDE;
                break;
            case '%':
                Result = TOKEN_REMAINDER;
                break;
            case '&':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '&')
                {
                    InputStream.ignore();
                    Result = TOKEN_LOGICAL_AND;
                }
                else
                    Result = TOKEN_BITWISE_AND;
                break;
            case '^':
                Result = TOKEN_BITWISE_XOR;
                break;
            case '|':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '|')
                {
                    InputStream.ignore();
                    Result = TOKEN_LOGICAL_OR;
                }
                else
                    Result = TOKEN_BITWISE_OR;
                break;
            case '~':
                Result = TOKEN_BITWISE_NOT;
                break;
            case '=':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
                    InputStream.ignore();
                Result = TOKEN_EQUAL;
                break;
            case '!':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
                {
                    InputStream.ignore();
                    Result = TOKEN_NOT_EQUAL;
                }
                else
                    Result =  TOKEN_LOGICAL_NOT;
                break;
            case '$':
                IntegerValue = 0;
                if(!isxdigit(InputStream.peek()))
                    Result = TOKEN_DOLLAR;
                else
                {
                    while(!InputStream.eof() && !InputStream.fail() && isxdigit(InputStream.peek()))
                    {
                        char c = InputStream.get();
                        int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                        IntegerValue = (IntegerValue << 4) + v;
                    }
                    Result = TOKEN_NUMBER;
                }
                break;
            case '<':
                if(!InputStream.eof() && !InputStream.fail())
                    switch(InputStream.peek())
                    {
                        case '<':
                            InputStream.ignore();
                            Result = TOKEN_SHIFT_LEFT;
                            break;
                        case '=':
                            InputStream.ignore();
                            Result = TOKEN_LESS_OR_EQUAL;
                            break;
                        default:
                            Result = TOKEN_LESS;
                            break;
                    }
                break;
            case '>':
                if(!InputStream.eof() && !InputStream.fail())
                    switch(InputStream.peek())
                    {
                        case '>':
                            InputStream.ignore();
                            Result = TOKEN_SHIFT_RIGHT;
                            break;
                        case '=':
                            InputStream.ignore();
                            Result = TOKEN_GREATER_OR_EQUAL;
                            break;
                        default:
                            Result = TOKEN_GREATER;
                            break;
                    }
                break;
            case '\'':
                if(InputStream.eof() || InputStream.fail())
                    throw ExpressionException("Unterminated character constant");

                if(InputStream.peek() == '\'')
                    throw ExpressionException("Empty Character constant");

                if(InputStream.peek() == '\\')
                {
                    InputStream.ignore();
                    if(InputStream.eof() || InputStream.fail())
                        throw ExpressionException("Unterminated character constant");
                    int EscapedChar = InputStream.get();
                    if(InputStream.eof() || InputStream.fail())
                        throw ExpressionException("Unterminated character constant");
                    if(InputStream.get() != '\'')
                        throw ExpressionException("Character constant too long");
                    switch(EscapedChar)
                    {
                        case '\'':
                            IntegerValue = 0x27;
                            break;
                        case '\"':
                            IntegerValue = 0x22;
                            break;
                        case '\?':
                            IntegerValue = 0x3F;
                            break;
                        case '\\':
                            IntegerValue = 0x5C;
                            break;
                        case 'a':
                            IntegerValue = 0x07;
                            break;
                        case 'b':
                            IntegerValue = 0x08;
                            break;
                        case 'f':
                            IntegerValue = 0x0C;
                            break;
                        case 'n':
                            IntegerValue = 0x0A;
                            break;
                        case 'r':
                            IntegerValue = 0x0D;
                            break;
                        case 't':
                            IntegerValue = 0x09;
                            break;
                        case 'v':
                            IntegerValue = 0x0B;
                            break;
                        default:
                            throw ExpressionException("Unrecognised escape sequence");
                            break;
                    }
                }
                else
                {
                    if(InputStream.eof() || InputStream.fail())
                        throw ExpressionException("Unterminated character constant");
                    IntegerValue = InputStream.get();
                    if(InputStream.eof() || InputStream.fail())
                        throw ExpressionException("Unterminated character constant");
                    if(InputStream.get() != '\'')
                    {
                        if(InputStream.eof() || InputStream.fail())
                            throw ExpressionException("Unterminated character constant");
                        else
                            throw ExpressionException("Character constant too long");
                    }
                }
                Result = TOKEN_NUMBER;
                break;
            default:
            {
                if(isalpha(FirstChar) || FirstChar=='_')  // LABEL
                {
                    StringValue = FirstChar;
                    while(!InputStream.eof() && !InputStream.fail() && (isalnum(InputStream.peek()) || InputStream.peek() == '_'))
                        StringValue.push_back(InputStream.get());
                    ToUpper(StringValue);
                    Result = TOKEN_LABEL;
                }
                else if(isdigit(FirstChar)) // NUMBER
                {
                    IntegerValue = 0;
                    if(FirstChar == '0')   // OCTAL OR HEX
                    {
                        if(!InputStream.eof() && !InputStream.fail() && (InputStream.peek() == 'x' || InputStream.peek() == 'X')) // HEX
                        {
                            InputStream.ignore();
                            while(!InputStream.eof() && !InputStream.fail() && isxdigit(InputStream.peek()))
                            {
                                char c = InputStream.get();
                                int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                                IntegerValue = (IntegerValue << 4) + v;
                            }
                            Result = TOKEN_NUMBER;
                        }
                        else    // OCTAL
                        {
                            while(!InputStream.eof() && !InputStream.fail() && isdigit(InputStream.peek()))
                            {
                                char c = InputStream.get();
                                int v = c - '0';
                                if(v > 7)
                                    throw ExpressionException("Invalid digit in Octal constant");
                                IntegerValue = (IntegerValue << 3) + v;
                            }
                            Result = TOKEN_NUMBER;
                        }
                    }
                    else // DECIMAL
                    {
                        IntegerValue = FirstChar - '0';
                        while(!InputStream.eof() && !InputStream.fail() && isdigit(InputStream.peek()))
                        {
                            char c = InputStream.get();
                            int v = c - '0';
                            IntegerValue = IntegerValue * 10 + v;
                        }
                        Result = TOKEN_NUMBER;
                    }
                }
                else if(InputStream.eof() || InputStream.fail())
                    Result = TOKEN_END;
                else
                    throw ExpressionException("Unrecognised token in expression");
            }
        }
    }
    return Result;
}
