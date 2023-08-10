#include "exceptions.h"
#include "opcode.h"
#include "sourcecodereader.h"
#include "utils.h"

const std::map<std::string, PreProcessorDirectiveEnum> PreProcessorDirectives = {
    { "define",      PP_define  },
    { "undef",       PP_undef   },
    { "undefine",    PP_undef   },
    { "if",          PP_if      },
    { "ifdef",       PP_ifdef   },
    { "ifdefined",   PP_ifdef   },
    { "ifndef",      PP_ifndef  },
    { "ifundef",     PP_ifndef  },
    { "ifundefined", PP_ifndef  },
    { "else",        PP_else    },
    { "endif",       PP_endif   },
    { "include",     PP_include },
    { "list",        PP_list    }
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
    bool inQuote = false;
    bool inEscape = false;

    std::string out;

    for(auto ch : in)
    {
        if (!inQuote && !inEscape)
        {
            if (ch == ';')
                break;

            switch(ch)
            {
            case '\"':
                inQuote = true;
                out.push_back(ch);
                continue;
            }
        }

        out.push_back(ch);

        if (inQuote && ch == '\\')
        {
            inEscape = true;
            continue;
        }
        if (inQuote && ch == '\"')
        {
            inQuote = false;
            continue;
        }
        if (inEscape)
        {
            inEscape = false;
            continue;
        }
    }
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
bool IsPreProcessorDirective(std::string& Line, PreProcessorDirectiveEnum& Directive, std::string& Expression)
{
    std::smatch MatchResult;
    if(regex_match(Line, MatchResult, std::regex(R"(^#(\w+)(\s+(.*))?$)")))
    {
        const std::string FirstToken = MatchResult[1];
        Expression = MatchResult[3];

        if (PreProcessorDirectives.find(FirstToken) != PreProcessorDirectives.end())
        {
            Directive = PreProcessorDirectives.at(FirstToken);
            return true;
        }
        throw AssemblyError("Unrecognised PreProcessor directive", SEVERITY_Warning);
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
    throw AssemblyError("Unterminated #if/#ifdef/#ifndef", SEVERITY_Warning);
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
        bool inQuotes = false;
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
                if(inQuotes)
                {
                    out += ch;;
                    Line.erase(0,1);

                    if(ch == '\\')
                        inEscape = true;

                    if(inQuotes && ch == '\"' && !inEscape)
                        inQuotes = false;
                }
                else
                {
                    if(ch == '\"')
                        inQuotes = true;

                    std::string FirstWord;
                    std::smatch MatchResult;
                    if(regex_match(Line, MatchResult, std::regex(R"(^(\w+).*)")))
                    {
                        FirstWord = MatchResult[1];
                        if(FirstWord == Define)
                        {
                            out += Defines[FirstWord]; // TODO Update to expand Macro Parameters if implemented
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
std::optional<OPCODE> ExpandTokens(const std::string& Line, std::string& Label, std::string& Mnemonic, std::vector<std::string>& OperandList)
{
    std::smatch MatchResult;
    if(regex_match(Line, MatchResult, std::regex(R"(^(((\.?\w+):\s*)|\s+)((\w+)(\s+(.*))?)?$)"))) // Label: OpCode Operands
    {
        // Extract Label, OpCode and Operands
        std::string Operands;
        Label = MatchResult[3];
        Mnemonic = MatchResult[5];

        if(Mnemonic.length() == 0)
            return {};

        std::transform(Mnemonic.begin(), Mnemonic.end(), Mnemonic.begin(), ::tolower);
        Operands = MatchResult[7];

        // Convert Mnemonic to uppercoase.
        for(char& c : Mnemonic)
            c = std::toupper(c);

        // Split the Operands into a vector
        bool inQuote = false;
        bool inEscape = false;
        bool SkipSpaces = false;
        std::string out;
        for(auto ch : Operands)
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
            if(!inQuote && !inEscape && ch == ',')
            {
                OperandList.push_back(regex_replace(out, std::regex(R"(\s+$)"), ""));
                out="";
                SkipSpaces = true;
                continue;
            }
            switch(ch)
            {
            case '\"':
                inQuote = !inQuote;
                break;
            case '\\':
                inEscape = true;
                break;
            }
            out.push_back(ch);
        }
        if(out.size() > 0)
            OperandList.push_back(regex_replace(out, std::regex(R"(\s+$)"), ""));

        OPCODE MachineWord;
        try
        {
            MachineWord = OpCode::Lookup.at(Mnemonic);
        }
        catch (std::out_of_range Ex)
        {
            throw AssemblyError("Unrecognised Mnemonic", SEVERITY_Error);
        }
        return MachineWord;
    }
    else
        throw AssemblyError("Unable to parse line", SEVERITY_Error);
}
