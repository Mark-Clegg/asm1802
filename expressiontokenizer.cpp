#include "expressiontokenizer.h"
#include "expressionexception.h"
#include "utils.h"

ExpressionTokenizer::ExpressionTokenizer()
{
}

//!
//! \brief ExpressionTokenizer::Initialize
//! \param Expression
//!
//! Initialise the tokenizer with the given string expression
//!
void ExpressionTokenizer::Initialize(std::string& Expression)
{
    InputStream.str(Expression);
    InputStream.seekg(0);
    InputStream.clear();
    PeekValid = false;
}

//!
//! \brief ExpressionTokenizer::Peek
//! \return
//!
//! Return the next token in the input stream without actually retrieving it.
//!
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

//!
//! \brief ExpressionTokenizer::Get
//! \return
//!
//! Scan and return the next token in the input stream.
//! If the token has a string or integer value, populate the
//! StringValue or IntegerValue properties.
//!
ExpressionTokenizer::TokenEnum ExpressionTokenizer::Get()
{
    PeekValid = false;
    TokenEnum Result;
    while(!InputStream.eof() && !InputStream.fail() && isspace(InputStream.peek()))
        InputStream.ignore();

    char FirstChar = InputStream.get();
    if(InputStream.eof() || InputStream.fail())
        Result = TokenEnum::TOKEN_END;
    else
    {
        switch(FirstChar)
        {
            case '"':
                StringValue = QuotedString();
                Result = TokenEnum::TOKEN_QUOTED_STRING;
                break;
            case '(':
                Result = TokenEnum::TOKEN_OPEN_BRACE;
                break;
            case ')':
                Result = TokenEnum::TOKEN_CLOSE_BRACE;
                break;
            case '.':
                Result = TokenEnum::TOKEN_DOT;
                break;
            case '+':
                Result = TokenEnum::TOKEN_PLUS;
                break;
            case '-':
                Result = TokenEnum::TOKEN_MINUS;
                break;
            case '*':
                Result = TokenEnum::TOKEN_MULTIPLY;
                break;
            case '/':
                Result = TokenEnum::TOKEN_DIVIDE;
                break;
            case '%':
                IntegerValue = 0;
                if(InputStream.peek() == '0' || InputStream.peek() == '1')
                {
                    while(!InputStream.eof() && !InputStream.fail() && (InputStream.peek() == '0' || InputStream.peek() == '1'))
                        IntegerValue = (IntegerValue << 1) + InputStream.get() - '0';
                    Result = TokenEnum::TOKEN_NUMBER;
                }
                else
                    Result = TokenEnum::TOKEN_REMAINDER;
                break;
            case '&':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '&')
                {
                    InputStream.ignore();
                    Result = TokenEnum::TOKEN_LOGICAL_AND;
                }
                else
                    Result = TokenEnum::TOKEN_BITWISE_AND;
                break;
            case '^':
                Result = TokenEnum::TOKEN_BITWISE_XOR;
                break;
            case '|':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '|')
                {
                    InputStream.ignore();
                    Result = TokenEnum::TOKEN_LOGICAL_OR;
                }
                else
                    Result = TokenEnum::TOKEN_BITWISE_OR;
                break;
            case '~':
                Result = TokenEnum::TOKEN_BITWISE_NOT;
                break;
            case '=':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
                    InputStream.ignore();
                Result = TokenEnum::TOKEN_EQUAL;
                break;
            case '!':
                if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
                {
                    InputStream.ignore();
                    Result = TokenEnum::TOKEN_NOT_EQUAL;
                }
                else
                    Result =  TokenEnum::TOKEN_LOGICAL_NOT;
                break;
            case '$':
                IntegerValue = 0;
                if(!isxdigit(InputStream.peek()))
                    Result = TokenEnum::TOKEN_DOLLAR;
                else
                {
                    while(!InputStream.eof() && !InputStream.fail() && isxdigit(InputStream.peek()))
                    {
                        char c = InputStream.get();
                        int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                        IntegerValue = (IntegerValue << 4) + v;
                    }
                    Result = TokenEnum::TOKEN_NUMBER;
                }
                break;
            case '<':
                if(!InputStream.eof() && !InputStream.fail())
                    switch(InputStream.peek())
                    {
                        case '<':
                            InputStream.ignore();
                            Result = TokenEnum::TOKEN_SHIFT_LEFT;
                            break;
                        case '=':
                            InputStream.ignore();
                            Result = TokenEnum::TOKEN_LESS_OR_EQUAL;
                            break;
                        default:
                            Result = TokenEnum::TOKEN_LESS;
                            break;
                    }
                break;
            case '>':
                if(!InputStream.eof() && !InputStream.fail())
                    switch(InputStream.peek())
                    {
                        case '>':
                            InputStream.ignore();
                            Result = TokenEnum::TOKEN_SHIFT_RIGHT;
                            break;
                        case '=':
                            InputStream.ignore();
                            Result = TokenEnum::TOKEN_GREATER_OR_EQUAL;
                            break;
                        default:
                            Result = TokenEnum::TOKEN_GREATER;
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
                Result = TokenEnum::TOKEN_NUMBER;
                break;
            default:
            {
                if(isalpha(FirstChar) || FirstChar=='_')  // LABEL
                {
                    StringValue = FirstChar;
                    while(!InputStream.eof() && !InputStream.fail() && (isalnum(InputStream.peek()) || InputStream.peek() == '_'))
                        StringValue.push_back(InputStream.get());
                    ToUpper(StringValue);
                    Result = TokenEnum::TOKEN_LABEL;
                }
                else if(isdigit(FirstChar)) // NUMBER
                {
                    IntegerValue = 0;

                    std::string ConstStr(1,FirstChar);
                    while(!InputStream.eof()
                            && !InputStream.fail()
                            && (isxdigit(InputStream.peek())
                                || tolower(InputStream.peek()) == 'x'
                                || tolower(InputStream.peek()) == 'h'
                                || tolower(InputStream.peek()) == 'd'
                                || tolower(InputStream.peek()) == 'o'
                                || tolower(InputStream.peek()) == 'b'))
                    {
                        char c = tolower(InputStream.get());
                        ConstStr.push_back(c);
                    }

                    if(std::regex_match(ConstStr, std::regex("^[01]+b$")))
                    {
                        // Binary (....b)
                        ConstStr.pop_back();
                        for(int i = 0; i < ConstStr.size(); i++)
                            IntegerValue = (IntegerValue << 1) + ConstStr[i] - '0';
                        Result = TokenEnum::TOKEN_NUMBER;
                    }
                    else if(std::regex_match(ConstStr, std::regex("^[0-9a-f]+h$")))
                    {
                        // Hexadeciman (....h)
                        ConstStr.pop_back();
                        for(int i = 0; i < ConstStr.size(); i++)
                        {
                            char c = ConstStr[i];
                            int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                            IntegerValue = (IntegerValue << 4) + v;
                        }
                        Result = TokenEnum::TOKEN_NUMBER;
                    }
                    else if(std::regex_match(ConstStr, std::regex("^[0-7]+o$")))
                    {
                        // Octal (....o)
                        ConstStr.pop_back();
                        for(int i = 0; i < ConstStr.size(); i++)
                            IntegerValue = (IntegerValue << 3) + ConstStr[i] - '0';
                        Result = TokenEnum::TOKEN_NUMBER;
                    }
                    else if(std::regex_match(ConstStr, std::regex("^[0-9]+d$")))
                    {
                        // Decimal (....d)
                        ConstStr.pop_back();
                        for(int i = 0; i < ConstStr.size(); i++)
                            IntegerValue = (IntegerValue * 10) + ConstStr[i] - '0';
                        Result = TokenEnum::TOKEN_NUMBER;
                    }
                    else if(std::regex_match(ConstStr, std::regex("^0[0-7]*$")))
                    {
                        // Octal (....)
                        for(int i = 0; i < ConstStr.size(); i++)
                            IntegerValue = (IntegerValue << 3) + ConstStr[i] - '0';
                        Result = TokenEnum::TOKEN_NUMBER;
                    }
                    else if(std::regex_match(ConstStr, std::regex("^0x[0-9a-f]+$")))
                    {
                        // Hexadecimal (0x....)
                        for(int i = 2; i < ConstStr.size(); i++)
                        {
                            char c = ConstStr[i];
                            int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                            IntegerValue = (IntegerValue << 4) + v;
                        }
                        Result = TokenEnum::TOKEN_NUMBER;
                    }
                    else if(std::regex_match(ConstStr, std::regex("^[1-9][0-9]*$")))
                    {
                        // Decimal (....)
                        for(int i = 0; i < ConstStr.size(); i++)
                            IntegerValue = (IntegerValue * 10) + ConstStr[i] - '0';
                        Result = TokenEnum::TOKEN_NUMBER;
                    }
                    else
                        throw ExpressionException("Invalid integer constant");
                }
                else if(InputStream.eof() || InputStream.fail())
                    Result = TokenEnum::TOKEN_END;
                else
                    throw ExpressionException("Unrecognised token in expression");
            }
        }
    }
    return Result;
}

//!
//! \brief ExpressionTokenizer::GetCustomToken
//! \param Pattern
//! \return
//!
//! For cases where a non-standard token can be accepted, GetCustomToken will scan the
//! input stream for a match to the given regular expression. The RE should begin with
//! ^ and end with .*. If a match is found, then StringValue is set to MatchResult[1]
//!
bool ExpressionTokenizer::GetCustomToken(std::regex Pattern)
{
    std::string Line;
    while(!InputStream.eof() && !InputStream.fail() && isspace(InputStream.peek()))
        InputStream.ignore();

    PeekValid = false;
    std::streampos SavedPos = InputStream.tellg();
    if(std::getline(InputStream, Line))
    {
        std::smatch MatchResult;
        if(regex_match(Line, MatchResult, Pattern))
        {
            StringValue = MatchResult[1];
            InputStream.clear();
            InputStream.seekg(SavedPos);
            InputStream.seekg(StringValue.size(), InputStream.cur);
            return true;
        }
        else
        {
            InputStream.clear();
            InputStream.seekg(SavedPos);
            return false;
        }
    }
    else
        return false;
}

//!
//! \brief ExpressionTokenizer::QuotedString
//! \return
//!
//! Scan for a quoted string, expanding escaped characters, returning the found
//! string content in StringValue
//!
std::string ExpressionTokenizer::QuotedString()
{
    std::string Result;
    int Len = 0;
    while(!InputStream.eof() && !InputStream.fail())
    {
        char ch = InputStream.get();
        if(ch == '\"')
            break;
        if(ch == '\\')
        {
            ch = InputStream.get();
            switch(ch)
            {
                case '\'':
                    Result += '\'';
                    break;
                case '\"':
                    Result += '\"';
                    break;
                case '\?':
                    Result += '\?';
                    break;
                case '\\':
                    Result += '\\';
                    break;
                case 'a':
                    Result += '\a';
                    break;
                case 'b':
                    Result += '\b';
                    break;
                case 'f':
                    Result += '\f';
                    break;
                case 'n':
                    Result += '\n';
                    break;
                case 'r':
                    Result += '\r';
                    break;
                case 't':
                    Result += '\t';
                    break;
                case 'v':
                    Result += '\v';
                    break;
                default:
                    throw ExpressionException("Unrecognised escape sequence in string constant");
                    break;
            }
        }
        else
            Result += ch;
        Len++;
    }
    return Result;
}
