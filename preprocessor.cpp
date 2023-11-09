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
    { "DEFINE",      PP_define  },
    { "UNDEF",       PP_undef   },
    { "IF",          PP_if      },
    { "IFDEF",       PP_ifdef   },
    { "IFNDEF",      PP_ifndef  },
    { "ELSE",        PP_else    },
    { "ELSEIF",      PP_elseif  },
    { "ENDIF",       PP_endif   },
    { "INCLUDE",     PP_include },
    { "ERROR",       PP_error   }
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
    // Pre-Define DEFINES for common alignments
    Defines["WORD"]  = "2";
    Defines["DWORD"] = "4";
    Defines["QWORD"] = "8";
    Defines["PAGE"]  = "256";

    std::time_t Now = std::time(nullptr);
    Defines["__DATE__"] = fmt::format("\"{:%b %d %Y}\"", fmt::localtime(Now));
    Defines["__TIME__"] = fmt::format("\"{:%H:%M:%S}\"", fmt::localtime(Now));
    Defines["__TIMESTAMP__"] = fmt::format("\"{:%a %b %d %H:%M:%S %Y}\"", fmt::localtime(Now));

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

                if(IsDirective(Line, Directive, Expression))
                {
                    fmt::println(OutputStream, "{Line}", fmt::arg("Line", Line));
                    switch(Directive)
                    {
                        case PP_define:
                        {
                            std::string key;
                            std::string value;
                            std::smatch MatchResult;
                            if(regex_match(Expression, MatchResult, std::regex(R"(^(\w+)\s+(.*)$)")))
                            {
                                key = MatchResult[1];
                                value = MatchResult[2];
                            }
                            else
                            {
                                key = Expression;
                                value = "1";
                            }
                            ToUpper(key);
                            Defines[key]=value;
                            break;
                        }
                        case PP_undef:
                        {
                            ToUpper(Expression);
                            Defines.erase(Expression);
                            break;
                        }
                        case  PP_if:
                        {
                            IfNestingLevel.top()++;
                            if(Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                            ToUpper(Expression);
                            PreProcessorExpressionEvaluator E;
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
                                if(SkipTo({ PP_else, PP_elseif, PP_endif }) == PP_endif)
                                {
                                    IfNestingLevel.top()--;
                                }
                            }
                            break;
                        }
                        case PP_ifdef:
                        {
                            IfNestingLevel.top()++;
                            if(Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                            ToUpper(Expression);
                            if(Defines.find(Expression) == Defines.end())
                            {
                                if(SkipTo({ PP_else, PP_elseif, PP_endif }) == PP_endif)
                                {
                                    IfNestingLevel.top()--;
                                }
                            }
                            break;
                        }
                        case PP_ifndef:
                        {
                            IfNestingLevel.top()++;
                            if(Expression.empty())
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                            ToUpper(Expression);
                            if(Defines.find(Expression) != Defines.end())
                            {
                                if(SkipTo({ PP_else, PP_elseif, PP_endif }) == PP_endif)
                                {
                                    IfNestingLevel.top()--;
                                }
                            }
                            break;
                        }
                        case PP_else:
                        case PP_elseif:
                        {
                            if(IfNestingLevel.top() <= 0)
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "#else without preceeding #if");
                            if(SkipTo({ PP_endif }) == PP_endif)
                            {
                                IfNestingLevel.top()--;
                            }
                            break;
                        }
                        case PP_endif:
                        {
                            if(IfNestingLevel.top() <= 0)
                                throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "#endif without preceeding #if");
                            IfNestingLevel.top()--;
                            break;
                        }
                        case PP_error:
                            throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, fmt::format("#error: {Message}", fmt::arg("Message", Expression)));
                            break;

                        case PP_include:
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
                    }
                }
                else
                    fmt::println(OutputStream, "{Line}", fmt::arg("Line", Line));
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
    Defines[Identifier] = Expression;
}

void PreProcessor::RemoveDefine(const std::string& Identifier)
{
    Defines.erase(Identifier);
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
                            if(FirstWord == Define.first)
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
                case PP_if:
                case PP_ifdef:
                case PP_ifndef:
                    Level++;
                    break;
                case PP_else:
                    if (Level == 0)
                    {
                        WriteLineMarker(OutputStream, SourceStreams.top().Name, SourceStreams.top().LineNumber);
                        fmt::println(OutputStream, "{Line}", fmt::arg("Line", RawLine));
                    }
                    break;
                case PP_endif:
                    if (Level == 0)
                    {
                        WriteLineMarker(OutputStream, SourceStreams.top().Name, SourceStreams.top().LineNumber);
                        fmt::println(OutputStream, "{Line}", fmt::arg("Line", RawLine));
                    }
                    else
                        Level--;
                    break;
                case PP_elseif:
                    if (Level == 0)
                    {
                        WriteLineMarker(OutputStream, SourceStreams.top().Name, SourceStreams.top().LineNumber);
                        fmt::println(OutputStream, "{Line}", fmt::arg("Line", RawLine));

                        if(Expression.empty())
                            throw PreProcessorException(SourceStreams.top().Name, SourceStreams.top().LineNumber, "Expected Espression");
                        ToUpper(Expression);
                        PreProcessorExpressionEvaluator E;
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
                            return SkipTo({ PP_else, PP_elseif, PP_endif });
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
