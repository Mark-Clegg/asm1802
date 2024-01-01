#include <filesystem>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <fstream>
#include <regex>
#include "preprocessor.h"
#include "preprocessorexception.h"
#include "preprocessorexpressionevaluator.h"
#include "expressionexception.h"
#include "utils.h"

namespace fs = std::filesystem;

const std::map<std::string, PreProcessor::DirectiveEnum> PreProcessor::Directives =
{
    { "CPU",         DirectiveEnum::PP_processor },
    { "PROCESSOR",   DirectiveEnum::PP_processor },
    { "DEFINE",      DirectiveEnum::PP_define    },
    { "UNDEF",       DirectiveEnum::PP_undef     },
    { "IF",          DirectiveEnum::PP_if        },
    { "IFDEF",       DirectiveEnum::PP_ifdef     },
    { "IFNDEF",      DirectiveEnum::PP_ifndef    },
    { "ELSE",        DirectiveEnum::PP_else      },
    { "ELIF",        DirectiveEnum::PP_elif      },
    { "ENDIF",       DirectiveEnum::PP_endif     },
    { "INCLUDE",     DirectiveEnum::PP_include   },
    { "ERROR",       DirectiveEnum::PP_error     },
    { "LIST",        DirectiveEnum::PP_list      },
    { "SYMBOLS",     DirectiveEnum::PP_symbols   }
};

//!
//! \brief PreProcessor::SourceEntry::SourceEntry
//! \param Name
//!
//! Stack Entry for #include'd files
//!
PreProcessor::SourceEntry::SourceEntry(const std::string& Name) :
    Name(Name),
    LineNumber(0)
{
    Stream = new std::ifstream(Name);
}

//!
//! \brief PreProcessor::PreProcessor
//!
//! Constructor
//!
//! Pre-Define standard #define's
//!
PreProcessor::PreProcessor()
{
    std::time_t Now = std::time(nullptr);
    Defines["__DATE__"] = fmt::format("\"{:%b %d %Y}\"", fmt::localtime(Now));
    Defines["__TIME__"] = fmt::format("\"{:%H:%M:%S}\"", fmt::localtime(Now));
    Defines["__TIMESTAMP__"] = fmt::format("\"{:%a %b %d %H:%M:%S %Y}\"", fmt::localtime(Now));
}

void PreProcessor::SetCPU(CPUTypeEnum Processor)
{
    this->Processor = Processor;
}

//!
//! \brief PreProcessor::Run
//! \param InputFile
//! \param OutputFile
//! \return
//!
//! Run the Pre-Processor on InputFile, creating a temporary Pre-Processed file, and return the resultant filename in OutputFile
bool PreProcessor::Run(const std::string& InputFile, std::string& OutputFile)
{
    auto p = fs::path(InputFile);
    p.replace_extension("pp");
    OutputFile = p;

    SourceEntry Entry(InputFile);
    SourceStreams.push(Entry);

    if(!SourceStreams.top().Stream->good())
        return false;

    OutputStream = std::ofstream();
    OutputStream.open(OutputFile, std::ofstream::out | std::ofstream::trunc);

    if(!OutputStream.good())
        return false;

    // Setup stack of #if results
    std::stack<int> IfNestingLevel;
    IfNestingLevel.push(0);

    std::string Line;
    while(SourceStreams.size() > 0)
    {
        WriteLineMarker(OutputStream, SourceStreams.top().Name, SourceStreams.top().LineNumber + 1);
        Defines["__FILE__"] = fmt::format("\"{FileName}\"", fmt::arg("FileName", SourceStreams.top().Name));
        while(std::getline(*SourceStreams.top().Stream, Line))
        {
            SourceStreams.top().LineNumber++;
            try
            {
                Defines["__LINE__"] = fmt::format("{LineNumber}", fmt::arg("LineNumber", SourceStreams.top().LineNumber));
                ExpandDefines(Line);

                // remove last character if blank (<cr>/<lf>/<space>/<tab>
                while(Line.size() > 0 && (Line[Line.size()-1] == '\r' || Line[Line.size()-1] == '\n' || Line[Line.size()-1] == ' ' || Line[Line.size()-1] == '\t'))
                    Line.pop_back();

                DirectiveEnum Directive;
                std::string Expression;

                fmt::println(OutputStream, "{Line}", fmt::arg("Line", Line));
                if(IsDirective(Line, Directive, Expression))
                {
                    switch(Directive)
                    {
                        case DirectiveEnum::PP_define:
                        {
                            std::string key;
                            std::string value;
                            std::smatch MatchResult;
                            if(regex_match(Expression, MatchResult, std::regex(R"(^([_[:alpha:]][_.[:alnum:]]*)\s+(.*)$)")))
                            {
                                key = MatchResult[1];
                                value = MatchResult[2];
                            }
                            else if(regex_match(Expression, MatchResult, std::regex(R"(^([_[:alpha:]][_.[:alnum:]]*)$)")))
                            {
                                key = Expression;
                                value = "1";
                            }
                            else
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected #define key {value}");
                            ToUpper(key);
                            Defines[key]=value;
                            break;
                        }
                        case DirectiveEnum::PP_undef:
                        {
                            std::smatch MatchResult;
                            if(regex_match(Expression, MatchResult, std::regex(R"(^([_[:alpha:]][_.[:alnum:]]*)$)")))
                            {
                                std::string key = MatchResult[1];
                                ToUpper(key);
                                Defines.erase(key);
                            }
                            else
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Invalid variable name");
                            break;
                        }
                        case  DirectiveEnum::PP_if:
                        {
                            ElseCounters.push(0);
                            IfNestingLevel.top()++;
                            if(Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                            //ToUpper(Expression);
                            PreProcessorExpressionEvaluator E(Processor);
                            int Result;
                            try
                            {
                                Result = E.Evaluate(Expression);
                            }
                            catch (ExpressionException Message)
                            {
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, Message.what());
                            }

                            if(Result == 0)
                            {
                                if(SkipTo({ DirectiveEnum::PP_else, DirectiveEnum::PP_elif, DirectiveEnum::PP_endif }) == DirectiveEnum::PP_endif)
                                {
                                    IfNestingLevel.top()--;
                                }
                            }
                            break;
                        }
                        case DirectiveEnum::PP_ifdef:
                        {
                            ElseCounters.push(0);
                            IfNestingLevel.top()++;
                            if(Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                            ToUpper(Expression);
                            if(Defines.find(Expression) == Defines.end())
                            {
                                if(SkipTo({ DirectiveEnum::PP_else, DirectiveEnum::PP_elif, DirectiveEnum::PP_endif }) == DirectiveEnum::PP_endif)
                                {
                                    IfNestingLevel.top()--;
                                }
                            }
                            break;
                        }
                        case DirectiveEnum::PP_ifndef:
                        {
                            ElseCounters.push(0);
                            IfNestingLevel.top()++;
                            if(Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                            ToUpper(Expression);
                            if(Defines.find(Expression) != Defines.end())
                            {
                                if(SkipTo({ DirectiveEnum::PP_else, DirectiveEnum::PP_elif, DirectiveEnum::PP_endif }) == DirectiveEnum::PP_endif)
                                {
                                    IfNestingLevel.top()--;
                                }
                            }
                            break;
                        }
                        case DirectiveEnum::PP_else:
                            if(IfNestingLevel.top() <= 0)
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "#else without preceeding #if");
                            if(ElseCounters.top() != 0)
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Too many #else statements");
                            ElseCounters.top()++;
                            if(!Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Extra characters after #else");
                            if(SkipTo({ DirectiveEnum::PP_endif }) == DirectiveEnum::PP_endif)
                            {
                                IfNestingLevel.top()--;
                            }
                            break;
                        case DirectiveEnum::PP_elif:
                        {
                            if(IfNestingLevel.top() <= 0)
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "#elif without preceeding #if");
                            if(ElseCounters.top() != 0)
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "#elif must come before #else");
                            if(SkipTo({ DirectiveEnum::PP_endif }) == DirectiveEnum::PP_endif)
                            {
                                IfNestingLevel.top()--;
                            }
                            break;
                        }
                        case DirectiveEnum::PP_endif:
                        {
                            if(IfNestingLevel.top() <= 0)
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "#endif without preceeding #if");
                            if(!Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Extra characters after #endif");
                            ElseCounters.pop();
                            IfNestingLevel.top()--;
                            break;
                        }
                        case DirectiveEnum::PP_error:
                            throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, fmt::format("#error: {Message}", fmt::arg("Message", Expression)));
                            break;

                        case DirectiveEnum::PP_include:
                        {
                            std::smatch MatchResult;
                            if(regex_match(Expression, MatchResult, std::regex(R"(^[<\"]([^>\"]+)[>\"]$)")))
                            {
                                if(SourceStreams.size() > 100)
                                    throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Source File Nesting limit exceeded");
                                SourceEntry Entry(MatchResult[1]);
                                SourceStreams.push(Entry);
                                WriteLineMarker(OutputStream, SourceStreams.top().Name, 1);
                                IfNestingLevel.push(0);
                            }
                            else
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Unable to interpret filename expected <filename> or \"filename\"");
                            break;
                        }
                        case DirectiveEnum::PP_processor: // Check syntax, save value, and pass through to main assembler
                        {
                            std::string Operand = Expression;
                            ToUpper(Operand);
                            auto CPU = OpCodeTable::CPUTable.find(Operand);
                            if(CPU == OpCodeTable::CPUTable.end())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Unknown processor specification");
                            Processor = CPU->second;
                            break;
                        }
                        case DirectiveEnum::PP_list: // Check syntax and just pass through to main assembler
                            OnOffCheck(Expression);
                            break;
                        case DirectiveEnum::PP_symbols: // Check syntax and just pass through to main assembler
                            OnOffCheck(Expression);
                            break;
                    }
                }
            }
            catch (PreProcessorException Ex)
            {
                fmt::println("PreProcessor Error: {FileName}:{LineNumber} - {Message}", fmt::arg("FileName", Ex.FileName), fmt::arg("LineNumber", Ex.LineNumber), fmt::arg("Message", Ex.what()));
                ErrorCount++;
            }
        }

        try
        {
            if(IfNestingLevel.top() != 0)
            {
                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, fmt::format("PreProcessor Error: {FileName} - Unterminated #if/#ifdef/#ifndef", fmt::arg("FileName", SourceStreams.top().Name)));
                ErrorCount++;
            }
        }
        catch (PreProcessorException Ex)
        {
            fmt::println("PreProcessor Error: {FileName}:{LineNumber} - {Message}", fmt::arg("FileName", Ex.FileName), fmt::arg("LineNumber", Ex.LineNumber), fmt::arg("Message", Ex.what()));
            ErrorCount++;
        }
        IfNestingLevel.pop();
        SourceStreams.top().Stream->close();
        delete SourceStreams.top().Stream;
        SourceStreams.pop();
    }

    OutputStream.close();
    return ErrorCount == 0;
}

void PreProcessor::WriteLineMarker(std::ofstream& Output, const std::string& FileName, const int LineNumber)
{
    fmt::println(Output, "#line \"{FileName}\" {LineNumber}", fmt::arg("FileName", FileName), fmt::arg("LineNumber", LineNumber));
}

bool PreProcessor::IsDirective(const std::string& Line, DirectiveEnum& Directive, std::string& Expression)
{
    std::smatch MatchResult;
    std::string TrimmedLine = Trim(Line);
    if(regex_match(TrimmedLine, MatchResult, std::regex(R"(^#(\w+)(\s+(.*))?$)")))
    {
        std::string FirstToken = MatchResult[1];
        ToUpper(FirstToken);
        Expression = MatchResult[3];

        if (Directives.find(FirstToken) != Directives.end())
        {
            Directive = Directives.at(FirstToken);
            return true;
        }
        throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Unrecognised PreProcessor directive");
    }
    return false;
}

void PreProcessor::AddDefine(const std::string& Identifier, const std::string& Expression)
{
    std::string key = Identifier;
    ToUpper(key);
    Defines[key] = Expression;
}

void PreProcessor::RemoveDefine(const std::string& Identifier)
{
    std::string key = Identifier;
    ToUpper(key);
    Defines.erase(key);
}

void PreProcessor::OnOffCheck(const std::string& Operand)
{
    std::smatch MatchResult;
    std::string State = Operand;
    ToUpper(State);
    if(regex_match(State, MatchResult, std::regex(R"-(^(ON|OFF)$)-")) && (MatchResult[1] == "ON" || MatchResult[1] == "OFF"))
        return;
    else
        throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected \"ON\" or \"OFF\"");
}

//!
//! \brief ExpandDefines
//! \param Line
//! \param Defines
//!
//! Parse the given line, replacing any defines with their values
//! Defines are applied in order of definition, snd are cumulative
//!
void PreProcessor::ExpandDefines(std::string& Line)
{
    for(auto& Define : Defines)
    {
        std::string out;
        bool inSingleQuotes = false;
        bool inDoubleQuotes = false;
        bool inEscape = false;
        for(int i = 0; i < Line.size(); i++)
        {
            char ch = Line[i];

            if(inEscape)
            {
                out += ch;
                inEscape = false;
            }
            else if(inSingleQuotes)
            {
                out += ch;

                if(ch == '\\')
                    inEscape = true;

                if(inSingleQuotes && ch == '\'' && !inEscape)
                    inSingleQuotes = false;
            }
            else if(inDoubleQuotes)
            {
                out += ch;;

                if(ch == '\\')
                    inEscape = true;

                if(inDoubleQuotes && ch == '\"' && !inEscape)
                    inDoubleQuotes = false;
            }
            else if(ch == '\'')
            {
                inSingleQuotes = true;
                out += ch;
            }
            else if(ch == '\"')
            {
                inDoubleQuotes = true;
                out += ch;
            }
            else
            {
                std::string FirstWord;
                int j = i;
                while(isalnum(Line[j]) || Line[j]=='_')
                    FirstWord += Line[j++];
                std::string Lookup = FirstWord;
                ToUpper(Lookup);
                if(FirstWord.size() > 0)
                {
                    if(Lookup == Define.first)
                    {
                        out += Define.second;
                    }
                    else
                        out += FirstWord;
                    i += FirstWord.size() - 1;
                }
                else
                    out += Line[i];
            }
        }
        Line = out;
    }
}

//!
//! \brief SkipLines
//! \param Source
//! \return
//!
//! Read and ignore lines between #if/#else/#endif directives. Returns last directive read (#else or #end)
//!
PreProcessor::DirectiveEnum PreProcessor::SkipTo(const std::set<DirectiveEnum>& Directives)
{
    std::string RawLine;
    DirectiveEnum Directive;
    std::string Expression;
    int Level = 0;
    while (std::getline(*SourceStreams.top().Stream, RawLine))
    {
        SourceStreams.top().LineNumber++;
        std::string Line = Trim(RawLine);
        if (Line.size() > 0 && IsDirective(Line, Directive, Expression))
        {
            switch (Directive)
            {
                case DirectiveEnum::PP_if:
                case DirectiveEnum::PP_ifdef:
                case DirectiveEnum::PP_ifndef:
                    Level++;
                    break;
                case DirectiveEnum::PP_else:
                    if (Level == 0)
                    {
                        if(ElseCounters.top() != 0)
                            throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Too many #else statements");
                        ElseCounters.top()++;
                        WriteLineMarker(OutputStream, SourceStreams.top().Name, SourceStreams.top().LineNumber);
                        fmt::println(OutputStream, "{Line}", fmt::arg("Line", RawLine));
                    }
                    break;
                case DirectiveEnum::PP_endif:
                    if (Level == 0)
                    {
                        WriteLineMarker(OutputStream, SourceStreams.top().Name, SourceStreams.top().LineNumber);
                        fmt::println(OutputStream, "{Line}", fmt::arg("Line", RawLine));
                        ElseCounters.pop();
                    }
                    else
                        Level--;
                    break;
                case DirectiveEnum::PP_elif:
                    if (Level == 0)
                    {
                        if(ElseCounters.top() != 0)
                            throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "#elif must come before #else");
                        WriteLineMarker(OutputStream, SourceStreams.top().Name, SourceStreams.top().LineNumber);
                        fmt::println(OutputStream, "{Line}", fmt::arg("Line", RawLine));

                        if(Expression.empty())
                            throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                        ToUpper(Expression);
                        PreProcessorExpressionEvaluator E(Processor);
                        int Result;
                        try
                        {
                            Result = E.Evaluate(Expression);
                        }
                        catch (ExpressionException Message)
                        {
                            throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, Message.what());
                        }

                        if(Result == 0)
                            return SkipTo({ DirectiveEnum::PP_else, DirectiveEnum::PP_elif, DirectiveEnum::PP_endif });
                        break;
                    }
                // The remaining directives have no effect on if/else/end processing
                default:
                    break;
            }
            if(Directives.find(Directive) != Directives.cend())
                return Directive;
        }
    }
    throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Unterminated #if/#ifdef/#ifndef");
}
