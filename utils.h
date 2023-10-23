#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include "definemap.h"
#include "macro.h"
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
    PP_include
};

std::string trim(const std::string &);
bool IsPreProcessorDirective(const std::string& Line, PreProcessorDirectiveEnum& Directive, std::string& Expression);
PreProcessorDirectiveEnum SkipLines(SourceCodeReader& Source, std::string& TerminatingLine);
void ExpandDefines(std::string& Line, DefineMap& Defines);
const std::optional<OpCodeSpec> ExpandTokens(const std::string& Line, std::string& Label, std::string& OpCode, std::vector<std::string>& Operands);
void StringListToVector(std::string& Input, std::vector<std::string>& Output, char Delimiter);
std::string basename(const std::string FileName);
int AlignFromSize(int Size);
inline void ToUpper(std::string& In) { std::transform(In.begin(), In.end(), In.begin(), ::toupper); }
void StringToByteVector(const std::string& Operand, std::vector<uint8_t>& Data);
void ExpandMacro(const Macro& Definition, const std::vector<std::string>& Operands, std::string& Output);

#endif // UTILS_H
