#include "assemblyexception.h"
#include "opcodetable.h"
#include "sourcecodereader.h"
#include "utils.h"

const std::map<std::string, PreProcessorDirectiveEnum> PreProcessorDirectives = {
    { "DEFINE",      PP_define  },
    { "UNDEF",       PP_undef   },
    { "UNDEFINE",    PP_undef   },
    { "IF",          PP_if      },
    { "IFDEF",       PP_ifdef   },
    { "IFDEFINED",   PP_ifdef   },
    { "IFNDEF",      PP_ifndef  },
    { "IFUNDEF",     PP_ifndef  },
    { "IFUNDEFINED", PP_ifndef  },
    { "ELSE",        PP_else    },
    { "ENDIF",       PP_endif   },
    { "INCLUDE",     PP_include },
    { "LIST",        PP_list    },
    { "SYMBOLS",     PP_symbols }
};

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
    if(out.size() > 0 && (out[out.size()-1] == '\r' || out[out.size()-1] == '\n'))
        out.pop_back();
    return regex_replace(out, std::regex(R"(\s+$)"), "");
}

//!
//! \brief PreProcessorDirective
//! \param Line
//! \param Directive
//! \param expression
//! \return true if Line is a Pre-Processor directive
//!
//! Check if line is a pre-processor directive, parse and return
//!
bool IsPreProcessorDirective(const std::string& Line, PreProcessorDirectiveEnum& Directive, std::string& Expression)
{
    std::smatch MatchResult;
    if(regex_match(Line, MatchResult, std::regex(R"(^#(\w+)(\s+(.*))?$)")))
    {
        std::string FirstToken = MatchResult[1];
        ToUpper(FirstToken);
        Expression = MatchResult[3];

        if (PreProcessorDirectives.find(FirstToken) != PreProcessorDirectives.end())
        {
            Directive = PreProcessorDirectives.at(FirstToken);
            return true;
        }
        throw AssemblyException("Unrecognised PreProcessor directive", SEVERITY_Warning);
    }
    return false;
}

//!
//! \brief SkipLines
//! \param Source
//! \return
//!
//! Read and ignore lines between #if/#else/#end directives. Returns last directive read (#else or #end)
//!
PreProcessorDirectiveEnum SkipLines(SourceCodeReader& Source, std::string& TerminatingLine)
{
    std::string OriginalLine;
    PreProcessorDirectiveEnum Directive;
    std::string Expression;
    int Level = 0;
    while (Source.getLine(OriginalLine))
    {
        std::string Line = trim(OriginalLine);
        if (Line.size() > 0 && IsPreProcessorDirective(Line, Directive, Expression))
        {
            switch (Directive)
            {
            case PP_if:
            case PP_ifdef:
            case PP_ifndef:
                Level++;
                break;
            case PP_else:
                if (Level == 0)
                {
                    TerminatingLine = OriginalLine;
                    return PP_else;
                }
                break;
            case PP_endif:
                if (Level == 0)
                {
                    TerminatingLine = OriginalLine;
                    return PP_endif;
                }
                else
                    Level--;
                break;
            // The remaining directives have no effect on if/else/end processing
            default:
                break;
            }
        }
    }
    throw AssemblyException("Unterminated #if/#ifdef/#ifndef", SEVERITY_Warning);
}

//!
//! \brief ExpandDefines
//! \param Line
//! \param Defines
//!
//! Parse the given line, replacing any defines with their values
//! Defines are applied in order of definition, snd are cumulative
//!
void ExpandDefines(std::string& Line, DefineMap& Defines)
{
    for(auto& Define : Defines)
    {
        std::string out;
        bool inSingleQuotes = false;
        bool inDoubleQuotes = false;
        bool inEscape = false;
        while(Line.size() > 0)
        {
            char ch = Line[0];

            if(inEscape)
            {
                out += ch;
                Line.erase(0,1);
                inEscape = false;
            }
            else
            {
                if(inSingleQuotes)
                {
                    out += ch;
                    Line.erase(0,1);

                    if(ch == '\\')
                        inEscape = true;

                    if(inSingleQuotes && ch == '\'' && !inEscape)
                        inSingleQuotes = false;
                }
                else
                {
                    if(inDoubleQuotes)
                    {
                        out += ch;;
                        Line.erase(0,1);

                        if(ch == '\\')
                            inEscape = true;

                        if(inDoubleQuotes && ch == '\"' && !inEscape)
                            inDoubleQuotes = false;
                    }
                    else
                    {
                        if(ch == '\'')
                            inSingleQuotes = true;
                        if(ch == '\"')
                            inDoubleQuotes = true;

                        std::string FirstWord;
                        std::smatch MatchResult;
                        if(regex_match(Line, MatchResult, std::regex(R"(^(\w+).*)")))
                        {
                            FirstWord = MatchResult[1];
                            if(FirstWord == Define)
                            {
                                out += Defines[FirstWord];
                            }
                            else
                                out += FirstWord;
                            Line.erase(0,FirstWord.size());
                        }
                        else
                        {
                            out += Line[0];
                            Line.erase(0,1);
                        }
                    }
                }
            }
        }
        Line = out;
    }
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
        ToUpper(Mnemonic);

        if(Mnemonic.length() == 0)
            return {};

        Operands = MatchResult[7];

        StringListToVector(Operands, OperandList, ',');

        OpCodeSpec MachineWord;
        try
        {
            MachineWord = OpCodeTable::OpCode.at(Mnemonic);
        }
        catch (std::out_of_range Ex)
        {
            MachineWord = { MACROEXPANSION, PSEUDO_OP, CPU_1802 };
        }
        return MachineWord;
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
            case '\'': Data.push_back(0x27); break;
            case '\"': Data.push_back(0x22); break;
            case '\?': Data.push_back(0x3F); break;
            case '\\': Data.push_back(0x5C); break;
            case 'a':  Data.push_back(0x07); break;
            case 'b':  Data.push_back(0x08); break;
            case 'f':  Data.push_back(0x0C); break;
            case 'n':  Data.push_back(0x0A); break;
            case 'r':  Data.push_back(0x0D); break;
            case 't':  Data.push_back(0x09); break;
            case 'v':  Data.push_back(0x0B); break;
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
            ToUpper(Identifier);
            if(Parameters.find(Identifier) != Parameters.end())
                Output += Parameters[Identifier];
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
