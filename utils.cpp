#include "assemblyexception.h"
#include "opcodetable.h"
#include "utils.h"

//!
//! \brief trim
//! \param in
//! \return
//!
//! Strip trailing spaces and comments from the line
//!
std::string trim(const std::string& in)
{
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool inEscape = false;

    std::string out;

    for(auto ch : in)
    {
        if (!inSingleQuote && !inDoubleQuote && !inEscape)
        {
            if (ch == ';')
                break;

            switch(ch)
            {
                case '\'':
                    if(!inDoubleQuote)
                    {
                        inSingleQuote = true;
                        out.push_back(ch);
                        continue;
                    }
                    break;
                case '\"':
                    if(!inSingleQuote)
                    {
                        inDoubleQuote = true;
                        out.push_back(ch);
                        continue;
                    }
                    break;
            }
        }

        out.push_back(ch);

        if ((inDoubleQuote || inSingleQuote) && ch == '\\')
        {
            inEscape = true;
            continue;
        }
        if (inSingleQuote && ch == '\'')
        {
            inSingleQuote = false;
            continue;
        }
        if (inDoubleQuote && ch == '\"')
        {
            inDoubleQuote = false;
            continue;
        }
        if (inEscape)
        {
            inEscape = false;
            continue;
        }
    }
    // remove last character if \n or \r (convert MS-DOS line endings)
    while(out.size() > 0 && (out[out.size()-1] == '\r' || out[out.size()-1] == '\n' || out[out.size()-1] == ' ' || out[out.size()-1] == '\t'))
        out.pop_back();
    return out;
}

//!
//! \brief ExpandTokens
//! \param Line
//! \param Label
//! \param OpCode
//! \param Operands
//!
//! Expand the source line into Label, OpCode, Operands
const std::optional<OpCodeSpec> ExpandTokens(const std::string& Line, std::string& Label, std::string& Mnemonic, std::vector<std::string>& OperandList)
{
    std::smatch MatchResult;
    if(regex_match(Line, MatchResult, std::regex(R"(^(((\w+):?\s*)|\s+)((\w+)(\s+(.*))?)?$)"))) // Label{:} OpCode Operands
    {
        // Extract Label, OpCode and Operands
        std::string Operands;
        Label = MatchResult[3];
        ToUpper(Label);
        if(!Label.empty() && !regex_match(Label, std::regex(R"(^[A-Z_][A-Z0-9_]*$)")))
            throw AssemblyException(fmt::format("Invalid Label: '{Label}'", fmt::arg("Label", Label)), SEVERITY_Error);
        Mnemonic = MatchResult[5];

        if(Mnemonic.length() == 0)
            return {};
        ToUpper(Mnemonic);

        Operands = MatchResult[7];

        StringListToVector(Operands, OperandList, ',');

        OpCodeSpec OpCode;
        try
        {
            OpCode = OpCodeTable::OpCode.at(Mnemonic);
        }
        catch (std::out_of_range Ex)  // If Mnemonic wasn't found in the OpCode table, it's possibly a Macro so return MACROEXPANSION
        {
            OpCode = { MACROEXPANSION, PSEUDO_OP, CPU_1802 };
        }
        return OpCode;
    }
    else
        throw AssemblyException("Unable to parse line", SEVERITY_Error);
}

void StringListToVector(std::string& Input, std::vector<std::string>& Output, char Delimiter)
{
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool inBrackets = false;
    bool inEscape = false;
    bool SkipSpaces = false;
    std::string out;
    for(auto ch : Input)
    {
        if(SkipSpaces && (ch == ' ' || ch == '\t'))
            continue;
        SkipSpaces = false;
        if(inEscape)
        {
            out.push_back(ch);
            inEscape = false;
            continue;
        }
        if(!inSingleQuote && !inDoubleQuote && !inEscape && !inBrackets && ch == Delimiter)
        {
            Output.push_back(regex_replace(out, std::regex(R"(\s+$)"), ""));
            out="";
            SkipSpaces = true;
            continue;
        }
        switch(ch)
        {
            case '\'':
                if(!inDoubleQuote)
                    inSingleQuote = !inSingleQuote;
                break;
            case '\"':
                if(!inSingleQuote)
                    inDoubleQuote = !inDoubleQuote;
                break;
            case '(':
                inBrackets = true;
                break;
            case ')':
                inBrackets = false;
                break;
            case '\\':
                inEscape = true;
                break;
        }
        out.push_back(ch);
    }
    if(out.size() > 0)
        Output.push_back(regex_replace(out, std::regex(R"(\s+$)"), ""));
}

//!
//! \brief AlignFromSize
//! Calculate the lowest power of two greater or equal to the given sixe
//! \param Size
//! \return
//!
int AlignFromSize(int Size)
{
    Size--;
    int Result = 1;
    while(Size > 0 && Result < 0x100)
    {
        Result <<= 1;
        Size >>= 1;
    }
    return Result;
}

//!
//! \brief StringToByteVector
//! Scan the passed quoted string, return a byte array, resolving escaped special characters
//! Assumes first character is double quote, and skips it.
//! \param Operand
//! \param Data
//!
void StringToByteVector(const std::string& Operand, std::vector<uint8_t>& Data)
{
    int Len = 0;
    bool QuoteClosed = false;
    for(int i = 1; i< Operand.size(); i++)
    {
        if(Operand[i] == '\"')
        {
            if(i != Operand.size() - 1)
                throw AssemblyException("Error parsing string constant", SEVERITY_Error);
            else
            {
                QuoteClosed = true;
                break;
            }
        }
        if(Operand[i] == '\\')
        {
            if(i >= Operand.size() - 2)
                throw AssemblyException("Incomplete escape sequence at end of string constant", SEVERITY_Error);
            i++;
            switch(Operand[i])
            {
                case '\'':
                    Data.push_back(0x27);
                    break;
                case '\"':
                    Data.push_back(0x22);
                    break;
                case '\?':
                    Data.push_back(0x3F);
                    break;
                case '\\':
                    Data.push_back(0x5C);
                    break;
                case 'a':
                    Data.push_back(0x07);
                    break;
                case 'b':
                    Data.push_back(0x08);
                    break;
                case 'f':
                    Data.push_back(0x0C);
                    break;
                case 'n':
                    Data.push_back(0x0A);
                    break;
                case 'r':
                    Data.push_back(0x0D);
                    break;
                case 't':
                    Data.push_back(0x09);
                    break;
                case 'v':
                    Data.push_back(0x0B);
                    break;
                default:
                    throw AssemblyException("Unrecognised escape sequence in string constant", SEVERITY_Error);
                    break;
            }
        }
        else
            Data.push_back(Operand[i]);
        Len++;
    }
    if(Len == 0)
        throw AssemblyException("String constant is empty", SEVERITY_Error);
    if(!QuoteClosed)
        throw AssemblyException("unterminated string constant", SEVERITY_Error);
}

//!
//! \brief ExpandMacro
//! Apply the Operands to the Macro Arguments, and return the Expanded string with the operands replaced.
//! \param Definition
//! \param Operands
//! \return
//!
void ExpandMacro(const Macro& Definition, const std::vector<std::string>& Operands, std::string& Output)
{
    if(Definition.Arguments.size() != Operands.size())
        throw AssemblyException(fmt::format("Incorrect number of arguments passed to macro. Received {In}, Expected {Out}", fmt::arg("In", Operands.size()), fmt::arg("Out", Definition.Arguments.size())), SEVERITY_Error);

    std::map<std::string, std::string> Parameters;
    for(int i=0; i < Definition.Arguments.size(); i++)
        Parameters[Definition.Arguments[i]] = Operands[i];

    std::string Input = Definition.Expansion;
    std::regex IdentifierRegex(R"(^([A-Za-z_][A-Za-z0-9_]*).*$)", std::regex::extended);
    std::smatch MatchResult;

    while(Input.size() > 0)
    {
        if(regex_match(Input, MatchResult, IdentifierRegex))
        {
            std::string Identifier = MatchResult[1];
            std::string UCIdentifier = Identifier;
            ToUpper(UCIdentifier);
            if(Parameters.find(UCIdentifier) != Parameters.end())
                Output += Parameters[UCIdentifier];
            else
                Output += Identifier;
            Input.erase(0, Identifier.size());
        }
        else
        {
            Output += Input[0];
            Input.erase(0, 1);
        }
    }
}
