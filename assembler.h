#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "assemblyexception.h"
#include "macro.h"
#include "opcodetable.h"

class Assembler
{
public:
    enum class PreProcessorControlEnum
    {
        PP_LINE,
        PP_PROCESSOR,
        PP_LIST,
        PP_SYMBOLS
    };
    const static std::map<std::string, PreProcessorControlEnum> PreProcessorControlLookup;

    enum class SubroutineOptionsEnum
    {
        SUBOPT_ALIGN,
        SUBOPT_STATIC,
        SUBOPT_PAD
    };
    const static std::map<std::string, SubroutineOptionsEnum> SubroutineOptionsLookup;

    enum class OutputFormatEnum
    {
        INTEL_HEX,
        IDIOT4,
        ELFOS,
        BIN
    };
    const static std::map<std::string, OutputFormatEnum> OutputFormatLookup;

    Assembler(const std::string& FileName, CPUTypeEnum& InitialProcessor, bool ListingEnabled, bool DumpSymbols, bool& NoRegisters, bool& NoPorts, const std::vector<OutputFormatEnum>& BinMode);
    bool Run();
private:
    const std::string& FileName;
    const CPUTypeEnum& InitialProcessor;
    bool ListingEnabled;
    bool DumpSymbols;
    const bool& NoRegisters;
    const bool& NoPorts;
    const std::vector<OutputFormatEnum>& BinMode;

    const std::optional<OpCodeSpec> ExpandTokens(const std::string& Line, std::string& Label, std::string& OpCode, std::vector<std::string>& Operands);
    void ExpandMacro(const Macro& Definition, const std::vector<std::string>& Operands, std::string& Output);
    std::string GetFileName(std::string Operand);
    void StringToByteVector(const std::string& Operand, std::vector<uint8_t>& Data);
    void StringListToVector(std::string& Input, std::vector<std::string>& Output, char Delimiter);
    int  AlignFromSize(int Size);
    bool SetAlignFromKeyword(std::string Alignment, long& Align);
    int  GetAlignExtraBytes(int ProgramCounter, int Align);
    void PrintError(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const std::string& Message, const AssemblyErrorSeverity Severity, const bool InMacro);
    void PrintError(const std::string& Message, AssemblyErrorSeverity Severity);
};

#endif // ASSEMBLER_H
