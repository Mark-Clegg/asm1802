#include "assemblyexception.h"
#include "assemblyexpressiontokenizer.h"
#include "utils.h"

AssemblyExpressionTokenizer::AssemblyExpressionTokenizer()
{
}

void AssemblyExpressionTokenizer::Initialize(const std::string& Expression)
{
    InputStream.str(Expression);
    InputStream.seekg(0);
    InputStream.clear();
}

TokenEnum AssemblyExpressionTokenizer::Peek()
{
    std::streampos SavedPos = InputStream.tellg();
    Get();
    InputStream.clear();
    InputStream.seekg(SavedPos);
    return ID;
}

TokenEnum AssemblyExpressionTokenizer::Get()
{
    while(!InputStream.eof() && !InputStream.fail() && isspace(InputStream.peek()))
        InputStream.ignore();

    char FirstChar = InputStream.get();
    if(InputStream.eof() || InputStream.fail())
    {
        ID = TOKEN_END;
        return ID;
    }

    switch(FirstChar)
    {
        case '(':
            ID = TOKEN_OPEN_BRACE;
            break;
        case ')':
            ID = TOKEN_CLOSE_BRACE;
            break;
        case '.':
            ID = TOKEN_DOT;
            break;
        case '+':
            ID = TOKEN_PLUS;
            break;
        case '-':
            ID = TOKEN_MINUS;
            break;
        case '*':
            ID = TOKEN_MULTIPLY;
            break;
        case '/':
            ID = TOKEN_DIVIDE;
            break;
        case '%':
            ID = TOKEN_REMAINDER;
            break;
        case '&':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '&')
            {
                InputStream.ignore();
                ID = TOKEN_LOGICAL_AND;
                break;
            }
            ID = TOKEN_BITWISE_AND;
            break;
        case '^':
            ID = TOKEN_BITWISE_XOR;
            break;
        case '|':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '|')
            {
                InputStream.ignore();
                ID = TOKEN_LOGICAL_OR;
                break;
            }
            ID = TOKEN_BITWISE_OR;
            break;
        case '~':
            ID = TOKEN_BITWISE_NOT;
            break;
        case '=':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
                InputStream.ignore();
            ID = TOKEN_EQUAL;
            break;
        case '!':
            if(!InputStream.eof() && !InputStream.fail() && InputStream.peek() == '=')
            {
                InputStream.ignore();
                ID = TOKEN_NOT_EQUAL;
                break;
            }
            ID = TOKEN_LOGICAL_NOT;
            break;
        case ',':
            ID = TOKEN_COMMA;
            break;
        case '$':
            IntegerValue = 0;
            if(!isxdigit(InputStream.peek()))
                ID = TOKEN_DOLLAR;
            else
            {
                while(!InputStream.eof() && !InputStream.fail() && isxdigit(InputStream.peek()))
                {
                    char c = InputStream.get();
                    int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                    IntegerValue = (IntegerValue << 4) + v;
                }
                ID = TOKEN_NUMBER;
            }
            break;
        case '<':
            if(!InputStream.eof() && !InputStream.fail())
                switch(InputStream.peek())
                {
                    case '<':
                        InputStream.ignore();
                        ID = TOKEN_SHIFT_LEFT;
                        break;
                    case '=':
                        InputStream.ignore();
                        ID = TOKEN_LESS_OR_EQUAL;
                        break;
                    default:
                        ID = TOKEN_LESS;
                        break;
                }
            break;
        case '>':
            if(!InputStream.eof() && !InputStream.fail())
                switch(InputStream.peek())
                {
                    case '>':
                        InputStream.ignore();
                        ID = TOKEN_SHIFT_RIGHT;
                        break;
                    case '=':
                        InputStream.ignore();
                        ID = TOKEN_GREATER_OR_EQUAL;
                        break;
                    default:
                        ID = TOKEN_GREATER;
                        break;
                }
            break;
        case '\'':
            if(InputStream.eof() || InputStream.fail())
                throw AssemblyException("Unterminated character constant", SEVERITY_Error);

            if(InputStream.peek() == '\'')
                throw AssemblyException("Empty Character constant", SEVERITY_Error);

            if(InputStream.peek() == '\\')
            {
                InputStream.ignore();
                if(InputStream.eof() || InputStream.fail())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                int EscapedChar = InputStream.get();
                if(InputStream.eof() || InputStream.fail())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                if(InputStream.get() != '\'')
                    throw AssemblyException("Character constant too long", SEVERITY_Error);
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
                        throw AssemblyException("Unrecognised escape sequence", SEVERITY_Error);
                        break;
                }
            }
            else
            {
                if(InputStream.eof() || InputStream.fail())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                IntegerValue = InputStream.get();
                if(InputStream.eof() || InputStream.fail())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                if(InputStream.get() != '\'')
                {
                    if(InputStream.eof() || InputStream.fail())
                        throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                    else
                        throw AssemblyException("Character constant too long", SEVERITY_Error);
                }
            }
            ID = TOKEN_NUMBER;
            break;
        default:
        {
            if(isalpha(FirstChar) || FirstChar=='_')  // LABEL
            {
                StringValue = FirstChar;
                while(!InputStream.eof() && !InputStream.fail() && (isalnum(InputStream.peek()) || InputStream.peek() == '_'))
                    StringValue.push_back(InputStream.get());
                ToUpper(StringValue);
                ID = TOKEN_LABEL;
                break;
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
                        ID = TOKEN_NUMBER;
                        break;
                    }
                    else    // OCTAL
                    {
                        while(!InputStream.eof() && !InputStream.fail() && isdigit(InputStream.peek()))
                        {
                            char c = InputStream.get();
                            int v = c - '0';
                            if(v > 7)
                                throw AssemblyException("Invalid digit in Octal constant", SEVERITY_Error);
                            IntegerValue = (IntegerValue << 3) + v;
                        }
                        ID = TOKEN_NUMBER;
                        break;
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
                    ID = TOKEN_NUMBER;
                    break;
                }
            }
            if(InputStream.eof() || InputStream.fail())
            {
                ID = TOKEN_END;
                break;
            }
            throw AssemblyException("Unrecognised token in expression", SEVERITY_Error);
        }
    }

    return ID;
}
