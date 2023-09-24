#include "assemblyexception.h"
#include "expressiontokenizer.h"

ExpressionTokenizer::ExpressionTokenizer()
{
}

void ExpressionTokenizer::Initialize(const std::string& Expression)
{
    InputStream.str(Expression);
//    InputStream.exceptions(std::istringstream::eofbit);
    InputStream.seekg(0);
    InputStream.clear();
}

TokenEnum ExpressionTokenizer::PeekToken()
{
    std::streampos SavedPos = InputStream.tellg();
    GetToken();
    InputStream.clear();
    InputStream.seekg(SavedPos);
    return ID;
}

TokenEnum ExpressionTokenizer::GetToken()
{
    while(!InputStream.eof() && isspace(InputStream.peek()))
        InputStream.ignore();

    if(InputStream.eof())
    {
        ID = TOKEN_END;
        return ID;
    }

    switch(char FirstChar = InputStream.get())
    {
    case '(':
        ID = TOKEN_OPENBRACE;
        break;
    case ')':
        ID = TOKEN_CLOSEBRACE;
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
        ID = TOKEN_AND;
        break;
    case '^':
        ID = TOKEN_XOR;
        break;
    case '|':
        ID = TOKEN_OR;
        break;
    default:
    {
        if(isalpha(FirstChar))  // LABEL
        {
            StringValue = FirstChar;
            while(!InputStream.eof() && isalnum(InputStream.peek()))
                StringValue.push_back(InputStream.get());
            ID = TOKEN_LABEL;
            break;
        }
        if(FirstChar == '$')    // HEX NUMBER
        {
            IntegerValue = 0;
            if(!isxdigit(InputStream.peek()))
            {
                ID = TOKEN_DOT;
                break;
            }
            else
            {
                while(!InputStream.eof() && isxdigit(InputStream.peek()))
                {
                    char c = InputStream.get();
                    int v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
                    IntegerValue = (IntegerValue << 4) + v;
                }
                ID = TOKEN_NUMBER;
                break;
            }
        }
        if(isdigit(FirstChar))  // NUMBER
        {
            IntegerValue = 0;
            if(FirstChar == '0')   // OCTAL OR HEX
            {
                if(!InputStream.eof() && InputStream.peek() == 'x') // HEX
                {
                    InputStream.ignore();
                    while(!InputStream.eof() && isxdigit(InputStream.peek()))
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
                    while(!InputStream.eof() && isdigit(InputStream.peek()))
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
                while(!InputStream.eof() && isdigit(InputStream.peek()))
                {
                    char c = InputStream.get();
                    int v = c - '0';
                    IntegerValue = IntegerValue * 10 + v;
                }
                ID = TOKEN_NUMBER;
                break;
            }
        }
        if(FirstChar == '<') // SHIFT LEFT
        {
            if(!InputStream.eof() && InputStream.peek() == '<')
            {
                InputStream.ignore();
                ID = TOKEN_SHIFTLEFT;
                break;
            }
        }
        if(FirstChar == '>') // SHIFT RIGHT
        {
            if(!InputStream.eof() && InputStream.peek() == '>')
            {
                InputStream.ignore();
                ID = TOKEN_SHIFTRIGHT;
                break;
            }
        }
        if(FirstChar == '\'') // CHARACTER CONSTANT
        {
            if(InputStream.eof())
                throw AssemblyException("Unterminated character constant", SEVERITY_Error);

            if(InputStream.peek() == '\'')
                throw AssemblyException("Empty Character constant", SEVERITY_Error);

            if(InputStream.peek() == '\\')
            {
                InputStream.ignore();
                if(InputStream.eof())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                int EscapedChar = InputStream.get();
                if(InputStream.eof())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                if(InputStream.get() != '\'')
                    throw AssemblyException("Character constant too long", SEVERITY_Error);
                switch(EscapedChar)
                {
                case '\'': IntegerValue = 0x27; break;
                case '\"': IntegerValue = 0x22; break;
                case '\?': IntegerValue = 0x3F; break;
                case '\\': IntegerValue = 0x5C; break;
                case 'a':  IntegerValue = 0x07; break;
                case 'b':  IntegerValue = 0x08; break;
                case 'f':  IntegerValue = 0x0C; break;
                case 'n':  IntegerValue = 0x0A; break;
                case 'r':  IntegerValue = 0x0D; break;
                case 't':  IntegerValue = 0x09; break;
                case 'v':  IntegerValue = 0x0B; break;
                default:
                    throw AssemblyException("Unrecognised escape sequence", SEVERITY_Error);
                    break;
                }
            }
            else
            {
                if(InputStream.eof())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                IntegerValue = InputStream.get();
                if(InputStream.eof())
                    throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                if(InputStream.get() != '\'')
                {
                    if(InputStream.eof())
                        throw AssemblyException("Unterminated character constant", SEVERITY_Error);
                    else
                        throw AssemblyException("Character constant too long", SEVERITY_Error);
                }
            }
            ID = TOKEN_NUMBER;
            break;
        }
        if(InputStream.eof())
        {
            ID = TOKEN_END;
            break;
        }
        throw AssemblyException("Unrecognised token in expression", SEVERITY_Error);
    }
    }
    return ID;
}
