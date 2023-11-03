#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <fstream>
#include <map>
#include <stack>
#include <string>
#include "definemap.h"

class PreProcessor
{
    enum DirectiveEnum
    {
        PP_define,
        PP_undef,
        PP_if,
        PP_ifdef,
        PP_ifndef,
        PP_else,
        PP_endif,
        PP_include,
        PP_error
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
    bool Run(const std::string& InputFile, std::string& OutputFile);
    void AddDefine(const std::string& Identifier, const std::string& Expression);
    void RemoveDefine(const std::string& Identifier);
private:
    std::stack<SourceEntry> SourceStreams;

    std::ofstream OutputStream;
    inline void WriteLineMarker(std::ofstream& Output, const std::string& FileName, const int LineNumber);

    DefineMap Defines;

    static const std::map<std::string, PreProcessor::DirectiveEnum> Directives;
    bool IsDirective(const std::string& Line, DirectiveEnum& Directive, std::string& Expression);
    void ExpandDefines(std::string& Line);
    DirectiveEnum SkipLines();
    bool Evaluate(std::string& Expression);
    int ErrorCount = 0;
};

#endif // PREPROCESSOR_H
