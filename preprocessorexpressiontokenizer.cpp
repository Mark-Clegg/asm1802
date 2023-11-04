#include "preprocessorexpressiontokenizer.h"
#include "preprocessorexpressionexception.h"
#include "utils.h"

PreProcessorExpressionTokenizer::PreProcessorExpressionTokenizer()
{
}

void PreProcessorExpressionTokenizer::Initialize(std::string& Expression)
{
    InputStream.str(Expression);
    InputStream.seekg(0);
    InputStream.clear();
}

TokenEnum PreProcessorExpressionTokenizer::Peek()
{
    std::streampos SavedPos = InputStream.tellg();
    TokenEnum ID = Get();
    InputStream.clear();
    InputStream.seekg(SavedPos);
    return ID;
}

TokenEnum PreProcessorExpressionTokenizer::Get()
{
    while(!InputStream.eof() && !InputStream.fail() && isspace(InputStream.peek()))
        InputStream.ignore();

    char FirstChar = InputStream.get();
    if(InputStream.eof() || InputStream.fail())
        return TOKEN_END;;

    switch(FirstChar)
    {
        case '(':
            return TOKEN_OPEN_BRACE;
        case ')':
            return TOKEN_CLOSE_BRACE;
        case '.':
            return TOKEN_DOT;
        case '+':
            return TOKEN_PLUS;
        case '-':
            return TOKEN_MINUS;
        case '*':
            return TOKEN_MULTIPLY;
        case '/':
            return TOKEN_DIVIDE;
        case '%':
            return TOKEN_REMAINDER;
        case '&':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '&')
            {
                InputStream.ignore();
                return TOKEN_LOGICAL_AND;
            }
            return TOKEN_BITWISE_AND;
        case '^':
            return TOKEN_BITWISE_XOR;
        case '|':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '|')
            {
                InputStream.ignore();
                return TOKEN_LOGICAL_OR;
            }
            return TOKEN_BITWISE_OR;
        case '~':
            return TOKEN_BITWISE_NOT;
        case '=':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
                InputStream.ignore();
            return TOKEN_EQUAL;
        case '!':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
            {
                InputStream.ignore();
                return TOKEN_NOT_EQUAL;
            }
            return TOKEN_LOGICAL_NOT;
        case '$':
            IntegerValue = 0;
            if(!isxdigit(InputStream.peek()))
                throw PreProcessorExpressionException("Expected Hexadecimal digit");
            while(!InputStream.eof() && !InputStream.fail() && isxdigit(InputStream.peek()))
            {
                char c = InputStream.get();
                int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                IntegerValue = (IntegerValue << 4) + v;
            }
            return TOKEN_NUMBER;
        case '<':
            if(!InputStream.eof() && !InputStream.fail())
                switch(InputStream.peek())
                {
                    case '<':
                        InputStream.ignore();
                        return TOKEN_SHIFT_LEFT;
                    case '=':
                        InputStream.ignore();
                        return TOKEN_LESS_OR_EQUAL;
                    default:
                        return TOKEN_LESS;
                }
            break;
        case '>':
            if(!InputStream.eof() && !InputStream.fail())
                switch(InputStream.peek())
                {
                    case '>':
                        InputStream.ignore();
                        return TOKEN_SHIFT_RIGHT;
                        break;
                    case '=':
                        InputStream.ignore();
                        return TOKEN_GREATER_OR_EQUAL;
                        break;
                    default:
                        return TOKEN_GREATER;
                        break;
                }
            break;
        case '\'':
            if(InputStream.eof() || InputStream.fail())
                throw PreProcessorExpressionException("Unterminated character constant");

            if(InputStream.peek() == '\'')
                throw PreProcessorExpressionException("Empty Character constant");

            if(InputStream.peek() == '\\')
            {
                InputStream.ignore();
                if(InputStream.eof() || InputStream.fail())
                    throw PreProcessorExpressionException("Unterminated character constant");
                int EscapedChar = InputStream.get();
                if(InputStream.eof() || InputStream.fail())
                    throw PreProcessorExpressionException("Unterminated character constant");
                if(InputStream.get() != '\'')
                    throw PreProcessorExpressionException("Character constant too long");
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
                        throw PreProcessorExpressionException("Unrecognised escape sequence");
                        break;
                }
            }
            else
            {
                if(InputStream.eof() || InputStream.fail())
                    throw PreProcessorExpressionException("Unterminated character constant");
                IntegerValue = InputStream.get();
                if(InputStream.eof() || InputStream.fail())
                    throw PreProcessorExpressionException("Unterminated character constant");
                if(InputStream.get() != '\'')
                {
                    if(InputStream.eof() || InputStream.fail())
                        throw PreProcessorExpressionException("Unterminated character constant");
                    else
                        throw PreProcessorExpressionException("Character constant too long");
                }
            }
            return TOKEN_NUMBER;
        default:
        {
            if(isalpha(FirstChar) || FirstChar=='_')  // LABEL
            {
                StringValue = FirstChar;
                while(!InputStream.eof() && !InputStream.fail() && (isalnum(InputStream.peek()) || InputStream.peek() == '_'))
                    StringValue.push_back(InputStream.get());
                ToUpper(StringValue);
                return TOKEN_LABEL;
            }
            if(isdigit(FirstChar))  // NUMBER
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
                        return TOKEN_NUMBER;
                    }
                    else    // OCTAL
                    {
                        while(!InputStream.eof() && !InputStream.fail() && isdigit(InputStream.peek()))
                        {
                            char c = InputStream.get();
                            int v = c - '0';
                            if(v > 7)
                                throw PreProcessorExpressionException("Invalid digit in Octal constant");
                            IntegerValue = (IntegerValue << 3) + v;
                        }
                        return TOKEN_NUMBER;
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
                    return TOKEN_NUMBER;
                }
            }
            if(InputStream.eof() || InputStream.fail())
                return TOKEN_END;
            throw PreProcessorExpressionException("Unrecognised token in expression");
        }
    }

    return TOKEN_END;

}
