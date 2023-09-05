#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include "definemap.h"
#include "opcodetable.h"
#include "sourcecodereader.h"

enum PreProcessorDirectiveEnum
{
    PP_define,
    PP_undef,
    PP_if,
    PP_ifdef,
    PP_ifndef,
    PP_else,
    PP_endif,
    PP_include,
    PP_list,
    PP_symbols
};

std::string trim(const std::string &);
bool IsPreProcessorDirective(std::string& Line, PreProcessorDirectiveEnum& Directive, std::string& Expression);
PreProcessorDirectiveEnum SkipLines(SourceCodeReader& Source, std::string& TerminatingLine);
void ExpandDefines(std::string& Line, DefineMap& Defines);
const std::optional<OpCodeSpec> ExpandTokens(const std::string& Line, std::string& Label, std::string& OpCode, std::vector<std::string>& Operands);
std::string basename(const std::string FileName);
#endif // UTILS_H
