#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <fstream>
#include <map>
#include <stack>
#include <string>
#include <set>
#include "opcodetable.h"

class PreProcessor
{
    enum class DirectiveEnum
    {
        PP_processor,
        PP_define,
        PP_undef,
        PP_if,
        PP_ifdef,
        PP_ifndef,
        PP_else,
        PP_elif,
        PP_elifdef,
        PP_elifndef,
        PP_endif,
        PP_include,
        PP_error,
        PP_list,
        PP_symbols
    };

    class SourceEntry
    {
    private:
    public:
        const std::string Name;
        int LineNumber;
        std::ifstream* Stream;

        SourceEntry(const std::string& Name);
    };

public:
    PreProcessor();
    void SetCPU(CPUTypeEnum Processor);
    bool Run(const std::string& InputFile, std::string& OutputFile);
    void AddDefine(const std::string& Identifier, const std::string& Expression);
    void RemoveDefine(const std::string& Identifier);
private:
    std::stack<SourceEntry> SourceStreams;
    std::stack<int> ElseCounters;

    std::ofstream OutputStream;
    inline void WriteLineMarker(std::ofstream& Output, const std::string& FileName, const int LineNumber);

    std::map<std::string, std::string> Defines;
    CPUTypeEnum Processor = CPUTypeEnum::CPU_1802;

    static const std::map<std::string, PreProcessor::DirectiveEnum> Directives;
    bool IsDirective(const std::string& Line, DirectiveEnum& Directive, std::string& Expression);
    void ExpandDefines(std::string& Line);
    DirectiveEnum SkipTo(const std::set<DirectiveEnum>& Directives);
    void OnOffCheck(const std::string& Operand);
    int ErrorCount = 0;
};

#endif // PREPROCESSOR_H
