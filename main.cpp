#include <cstring>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <getopt.h>
#include "binarywriter_idiot4.h"
#include "binarywriter_intelhex.h"
#include "errortable.h"
#include "assemblyexception.h"
#include "assemblyexpressionevaluator.h"
#include "expressionexception.h"
#include "listingfilewriter.h"
#include "opcodetable.h"
#include "preprocessor.h"
#include "sourcecodereader.h"
#include "symboltable.h"
#include "utils.h"

namespace fs = std::filesystem;

std::string Version("0.1");

enum PreProcessorControlEnum
{
    PP_LINE,
    PP_PROCESSOR,
    PP_LIST,
    PP_SYMBOLS
};

std::map<std::string, PreProcessorControlEnum> PreProcessorControlLookup =
{
    { "line",      PP_LINE      },
    { "processor", PP_PROCESSOR },
    { "list",      PP_LIST      },
    { "symbols",   PP_SYMBOLS   }
};

enum SubroutineOptionsEnum
{
    SUBOPT_ALIGN,
    SUBOPT_STATIC
};

std::map<std::string, SubroutineOptionsEnum> SubroutineOptionsLookup =
{
    { "ALIGN",  SUBOPT_ALIGN  },
    { "STATIC", SUBOPT_STATIC }
};

enum OutputFormatEnum
{
    INTEL_HEX,
    IDIOT4
};

std::map<std::string, OutputFormatEnum> OutputFormatLookup =
{
    { "INTEL_HEX", INTEL_HEX },
    { "IDIOT4",    IDIOT4    }
};

bool assemble(const std::string&, CPUTypeEnum InitialProcessor, bool ListingEnabled, bool DumpSymbols, OutputFormatEnum BinMode);
void PrintError(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const std::string& Message, const AssemblyErrorSeverity Severity, const bool InMacro);
void PrintError(const std::string& Message, AssemblyErrorSeverity Severity);
void ExpandMacro(const Macro& Definition, const std::vector<std::string>& Operands, std::string& Output);
void StringToByteVector(const std::string& Operand, std::vector<uint8_t>& Data);
void StringListToVector(std::string& Input, std::vector<std::string>& Output, char Delimiter);
int AlignFromSize(int Size);
const std::optional<OpCodeSpec> ExpandTokens(const std::string& Line, std::string& Label, std::string& OpCode, std::vector<std::string>& Operands);
bool SetAlignFromKeyword(std::string Alignment, int& Align);

bool NoRegisters = false;   // Suppress pre-defined Register equates
bool NoPorts     = false;   // Suppress pre-defined Port equates

//!
//! \brief main
//! \param argc
//! \param argv
//! \return
//!
//! MAIN
//!
int main(int argc, char **argv)
{
    option longopts[] =
    {
        { "cpu",                required_argument,  0, 'C' }, // Specify target CPU
        { "define",             required_argument,  0, 'D' }, // Define pre-processor variable
        { "undefine",           required_argument,  0, 'U' }, // Un-define pre-processor variable
        { "keep-preprocessor",  no_argument,        0, 'k' }, // Keep Pre-Processor intermediate file
        { "list",               no_argument,        0, 'l' }, // Create a listing file after pass 3
        { "symbols",            no_argument,        0, 's' }, // Include Symbol Table in listing file
        { "noregisters",        no_argument,        0, 'r' }, // Do not pre-define labels for Registers (R0-F, R0-15)
        { "noports",            no_argument,        0, 'p' }, // No not pre-define labels for Ports (P1-7)
        { "output",             required_argument,  0, 'o' }, // Set output file type (default = Intel Hex)
        { "version",            no_argument,        0, 'v' }, // Print version number and exit
        { "help",               no_argument,        0, '?' }, // Print using information
        { 0,0,0,0 }
    };

    std::string FileName = fs::path(argv[0]).filename();
    fmt::println("{FileName}: Version {Version}", fmt::arg("FileName", FileName), fmt::arg("Version", Version));
    fmt::println("Macro Assembler for the COSMAC CDP1802 series MicroProcessor");
    fmt::println("");

    CPUTypeEnum InitialProcessor = CPU_1802;
    bool Listing = false;
    PreProcessor AssemblerPreProcessor;
    bool KeepPreprocessor = false;
    bool Symbols = false;
    OutputFormatEnum OutputFormat = INTEL_HEX;

    while (1)
    {
        const int opt = getopt_long(argc, argv, "C:D:U:klso:v?", longopts, 0);

        if (opt == -1)
            break;

        switch (opt)
        {
            case 'C': // CPU Type
            {
                std::string RequestedCPU = optarg;
                ToUpper(RequestedCPU);
                auto CPULookup = OpCodeTable::CPUTable.find(RequestedCPU);
                if(CPULookup == OpCodeTable::CPUTable.end())
                    fmt::println("Unrecognised CPU Type");
                else
                    InitialProcessor = CPULookup->second;
                break;
            }
            case 'D': // Define Pre-Processor variable
            {
                std::string trimmedKvp = regex_replace(optarg, std::regex(R"(\s+$)"), "");
                std::string key;
                std::string value;
                std::smatch MatchResult;
                if(regex_match(trimmedKvp, MatchResult, std::regex(R"(^\s*(\w+)\s*=\s*(.*)$)")))
                {
                    key = MatchResult[1];
                    value = MatchResult[2];
                }
                else
                {
                    key = trimmedKvp;
                    value = "";
                }
                ToUpper(key);
                AssemblerPreProcessor.AddDefine(key, value);
                break;
            }

            case 'U': // UnDefine Pre-Processor variable
            {
                std::string key = optarg;
                ToUpper(key);
                AssemblerPreProcessor.RemoveDefine(optarg);
                break;
            }
            case 'k': // Keep Pre-Processor temporary file (.pp)
                KeepPreprocessor = true;
                break;

            case 'l': // Create Listing file (.lst)
                Listing = true;
                break;

            case 's': // Dump Symbol Table to Listing file
                Symbols = true;
                break;

            case 'r': // Do Not pre-define R0-RF
                NoRegisters = true;
                break;

            case 'p': // Do Not pre-define P1-P7
                NoPorts = true;
                break;

            case 'o': // Set Binary Output format
            {
                std::string Mode = optarg;
                ToUpper(Mode);
                if(OutputFormatLookup.find(Mode) == OutputFormatLookup.end())
                    fmt::println("** Unrecognised binary output mode. Defaulting to Intel Hex");
                else
                    OutputFormat = OutputFormatLookup.at(Mode);
                break;
            }
            case 'v': // Display Version number
            {
                fmt::println("{version}", fmt::arg("version", Version));
                return 0;
            }
            case '?': // Print Help
            {
                fmt::println("Usage:");
                fmt::println("asm1802 <options> SourceFile");
                fmt::println("");
                fmt::println("Options:");
                fmt::println("");
                fmt::println("-C|--cpu Processor");
                fmt::println("\tSpecify target CPU: 1802, 1804/5/6, 1804/5/6A");
                fmt::println("");
                fmt::println("-D|--define Name{{=value}}");
                fmt::println("\tDefine preprocessor variable");
                fmt::println("");
                fmt::println("-U|--undefine Name");
                fmt::println("\tUndefine preprocessor variable");
                fmt::println("");
                fmt::println("-k|--keep-preprocessor");
                fmt::println("\tKeep Pre-Processor temporary file {{filename}}.pp");
                fmt::println("");
                fmt::println("-l|--list");
                fmt::println("\tCreate listing file");
                fmt::println("");
                fmt::println("-s|--symbols");
                fmt::println("\tInclude Symbol Tables in listing");
                fmt::println("");
                fmt::println("--noregisters");
                fmt::println("\tDo not predefine R0-RF register symbols");
                fmt::println("");
                fmt::println("--noports");
                fmt::println("\tDo not predefine P1-P7 port symbols");
                fmt::println("");
                fmt::println("-o|--output format");
                fmt::println("\tCreate output file in \"intelhex\" or \"idiiot4\" format");
                fmt::println("");
                fmt::println("-v|--version");
                fmt::println("\tPrint version number and exit");
                fmt::println("");
                fmt::println("-?|--help");
                fmt::println("\tPrint thie help and exit");
                return 0;
            }
            default:
            {
                fmt::println("Error");
                return 1;
            }
        }
    }

    AssemblerPreProcessor.SetCPU(InitialProcessor);
    bool Result = false;
    if (optind + 1 ==  argc)
    {
        try
        {
            fmt::println("Pre-Processing...");
            std::string PreProcessedInputFile;
            std::string FileName = argv[optind++];
            if(AssemblerPreProcessor.Run(FileName, PreProcessedInputFile))
            {
                if(KeepPreprocessor)
                    fmt::println("Pre-Processed input saved to {FileName}", fmt::arg("FileName", PreProcessedInputFile));
                Result = assemble(PreProcessedInputFile, InitialProcessor, Listing, Symbols, OutputFormat);
            }
            else
            {
                fmt::println("Pre-Procssing Failed, Assembly Aborted");
                fmt::println("");
            }

            if(!KeepPreprocessor)
                std::remove(PreProcessedInputFile.c_str());
        }
        catch (AssemblyException Error)
        {
            fmt::println("** Error opening/reading file: {message}", fmt::arg("message", Error.what()));
        }
    }
    else
        fmt::println("Expected a single filename to assemble");

    return Result ? 0 : 1;
}

//!
//! \brief assemble
//! \param FileName
//! \return
//!
//! Main Assembler
//!
bool assemble(const std::string& FileName, CPUTypeEnum InitialProcessor, bool ListingEnabled, bool DumpSymbols, OutputFormatEnum BinMode)
{
    SymbolTable MainTable;
    std::map<std::string, SymbolTable> SubTables;
    std::map<uint16_t, std::vector<uint8_t>> Code;
    std::map<uint16_t, std::vector<uint8_t>>::iterator CurrentCode;
    std::optional<uint16_t> EntryPoint;
    std::set<std::string> UnReferencedSubs;

    // Pre-Define LABELS for Registers
    if(!NoRegisters)
        for(int i=0; i<16; i++)
        {
            MainTable.Symbols[fmt::format("R{n}",   fmt::arg("n", i))] = { i, true };
            MainTable.Symbols[fmt::format("R{n:X}", fmt::arg("n", i))] = { i, true };
        }
    // Pre-Define LABELS for Ports
    if(!NoPorts)
        for(int i=1; i<8; i++)
        {
            MainTable.Symbols[fmt::format("P{n}",   fmt::arg("n", i))] = { i, true };
        }

    ErrorTable Errors;
    ListingFileWriter ListingFile(FileName, Errors, ListingEnabled);

    for(int Pass = 1; Pass <= 3 && Errors.count(SEVERITY_Error) == 0; Pass++)
    {
        SymbolTable* CurrentTable = &MainTable;
        uint16_t ProgramCounter = 0;
        uint16_t SubroutineSize = 0;
        CPUTypeEnum Processor = InitialProcessor;
        std::string CurrentFile = "";
        std::string SubDefinitionFile = "";
        int LineNumber = 0;
        bool InSub = false;
        bool InAutoAlignedSub = false;

        Code.clear();
        Code = {{ 0, {}}};
        CurrentCode = Code.begin();

        try
        {
            fmt::println("Pass {pass}", fmt::arg("pass", Pass));

            // Setup Source File stack
            SourceCodeReader Source(FileName);

            // Setup stack of #if results
            int IfNestingLevel = 0;

            // Read and process each line
            std::string OriginalLine;
            while(Source.getLine(OriginalLine))
            {
                std::string Line = Trim(OriginalLine);

                // Check for Pre-Processor Control statement (#control expression...)
                std::smatch MatchResult;
                if(regex_match(Line, MatchResult, std::regex(R"-(^#(\w+)(\s+(.*))?$)-")))
                {
                    std::string ControlWord = MatchResult[1];
                    std::string Expression = MatchResult[3];
                    if(PreProcessorControlLookup.find(ControlWord) != PreProcessorControlLookup.end())
                    {
                        switch(PreProcessorControlLookup.at(ControlWord))
                        {
                            case PP_LINE:
                            {
                                std::smatch MatchResult;
                                std::string NewFile;
                                if(regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-")))
                                {
                                    CurrentFile = MatchResult[1];
                                    LineNumber = stoi(MatchResult[2]);
                                }
                                else
                                    throw AssemblyException("Bad line directive received from Pre-Processor", SEVERITY_Error);
                                break;
                            }
                            case PP_PROCESSOR:
                            {
                                if(Pass == 3)
                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                ToUpper(Expression);
                                if(OpCodeTable::CPUTable.find(Expression) != OpCodeTable::CPUTable.end())
                                    Processor = OpCodeTable::CPUTable.at(Expression);
                                else
                                    throw AssemblyException("Bad processor directive received from Pre-Processor", SEVERITY_Error);
                                if(!Source.InMacro())
                                    LineNumber++;
                                break;
                            }
                            case PP_LIST:
                                if(Pass == 3)
                                {
                                    ToUpper(Expression);
                                    if(Expression == "ON")
                                    {
                                        ListingFile.Enabled = true;
                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                    }
                                    else if(Expression == "OFF")
                                    {
                                        if(ListingFile.Enabled)
                                            ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                        ListingFile.Enabled = false;
                                    }
                                    else
                                        throw AssemblyException("Bad list directive received from Pre-Processor", SEVERITY_Error);
                                }
                                if(!Source.InMacro())
                                    LineNumber++;
                                break;
                            case PP_SYMBOLS:
                                if(Pass == 3)
                                {
                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                    ToUpper(Expression);
                                    if(Expression == "ON")
                                        DumpSymbols = true;
                                    else if(Expression == "OFF")
                                        DumpSymbols = false;
                                    else
                                        throw AssemblyException("Bad symbols directive received from Pre-Processor", SEVERITY_Error);
                                }
                                if(!Source.InMacro())
                                    LineNumber++;
                                break;
                        }
                        continue; // Go back to start of getLine loop - control statements have no further processing and are not included in the listing file.
                    }
                    if(Pass == 3)
                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                }
                else
                {
                    if (Line.size() > 0)
                    {
                        try
                        {
                            if(InSub && SubDefinitionFile != CurrentFile)
                            {
                                InSub = false;
                                throw AssemblyException("Subroutine definition must be within a single source file", SEVERITY_Error);
                            }
                            std::string Label;
                            std::string Mnemonic;
                            std::vector<std::string>Operands;
                            std::optional<OpCodeSpec> OpCode = ExpandTokens(Line, Label, Mnemonic, Operands);

                            switch(Pass)
                            {
                                case 1: // Calculate the size of subroutines
                                {
                                    if(OpCode)
                                    {
                                        if(OpCode.value().OpCodeType == PSEUDO_OP)
                                        {
                                            switch(OpCode.value().OpCode)
                                            {
                                                case SUB:
                                                    if(InSub)
                                                        throw AssemblyException("SUBROUTINEs cannot be nested", SEVERITY_Error, ENDSUB);

                                                    if(Label.empty())
                                                        throw AssemblyException("SUBROUTINE requires a Label", SEVERITY_Error, ENDSUB);

                                                    if(SubTables.find(Label) != SubTables.end())
                                                        throw AssemblyException(fmt::format("Subroutine '{Label}' is already defined", fmt::arg("Label", Label)), SEVERITY_Error, ENDSUB);

                                                    InSub = true;
                                                    SubDefinitionFile = CurrentFile;
                                                    SubTables.insert(std::pair<std::string, SymbolTable>(Label, SymbolTable()));
                                                    CurrentTable = &SubTables[Label];
                                                    SubroutineSize = 0;
                                                    break;
                                                case ENDSUB:
                                                {
                                                    if(!InSub)
                                                        throw AssemblyException("ENDSUB without matching SUB", SEVERITY_Error);
                                                    InSub = false;

                                                    CurrentTable->CodeSize = SubroutineSize;
                                                    CurrentTable = &MainTable;
                                                    break;
                                                }
                                                case MACRO:
                                                {
                                                    if(CurrentTable->Macros.find(Label) != CurrentTable->Macros.end())
                                                        throw AssemblyException(fmt::format("Macro '{Macro}' is already defined", fmt::arg("Macro", Label)), SEVERITY_Error, ENDMACRO);
                                                    if(OpCodeTable::OpCode.find(Label) != OpCodeTable::OpCode.end())
                                                        throw AssemblyException(fmt::format("Cannot use reserved word '{OpCode}' as a Macro name", fmt::arg("OpCode", Label)), SEVERITY_Error, ENDMACRO);
                                                    Macro& MacroDefinition = CurrentTable->Macros[Label];
                                                    std::regex ArgMatch(R"(^[A-Z_][A-Z0-9_]*$)");
                                                    for(auto& Arg : Operands)
                                                    {
                                                        std::string Argument(Arg);
                                                        ToUpper(Argument);

                                                        if(OpCodeTable::OpCode.find(Argument) != OpCodeTable::OpCode.cend())
                                                            throw AssemblyException(fmt::format("Cannot use reserved word '{OpCode}' as a Macro parameter", fmt::arg("OpCode", Argument)), SEVERITY_Error, ENDMACRO);

                                                        if(std::regex_match(Argument, ArgMatch))
                                                            if(std::find(MacroDefinition.Arguments.begin(), MacroDefinition.Arguments.end(), Argument) == MacroDefinition.Arguments.end())
                                                                MacroDefinition.Arguments.push_back(Argument);
                                                            else
                                                                throw AssemblyException("Macro arguments must be unique", SEVERITY_Error);
                                                        else
                                                            throw AssemblyException(fmt::format("Invalid argument name: '{Name}'", fmt::arg("Name", Argument)), SEVERITY_Error);
                                                    }

                                                    std::ostringstream Expansion;
                                                    while(Source.getLine(OriginalLine))
                                                    {
                                                        LineNumber++;
                                                        std::string Line = Trim(OriginalLine);

                                                        // Throw an error if the source file changes mid definition
                                                        if(regex_match(Line, MatchResult, std::regex(R"-(^#(\w+)(\s+(.*))?$)-")))
                                                        {
                                                            std::string ControlWord = MatchResult[1];
                                                            std::string Expression = MatchResult[3];
                                                            if(PreProcessorControlLookup.find(ControlWord) != PreProcessorControlLookup.end()
                                                                    && PreProcessorControlLookup.at(ControlWord) == PP_LINE
                                                                    && regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-"))
                                                                    && CurrentFile != MatchResult[1])
                                                                throw AssemblyException("Macro definition must be within a single source file", SEVERITY_Error);
                                                        }

                                                        std::optional<OpCodeSpec> OpCode;
                                                        try
                                                        {
                                                            OpCode = ExpandTokens(Line, Label, Mnemonic, Operands);
                                                        }
                                                        catch(AssemblyException Ex)
                                                        {
                                                            Label = {};
                                                            Mnemonic = {};
                                                            OpCode = {};
                                                            Operands = {};
                                                        }
                                                        if(!Label.empty())
                                                            throw AssemblyException("Cannot define a label inside a macro", SEVERITY_Error);
                                                        if(OpCode.has_value() && OpCode.value().OpCode == ENDMACRO)
                                                            break;
                                                        fmt::println(Expansion, OriginalLine);
                                                    }

                                                    MacroDefinition.Expansion = Expansion.str();
                                                    break;
                                                }
                                                case ENDMACRO:
                                                    throw AssemblyException("ENDMACRO without opening MACRO pseudo-op", SEVERITY_Error);
                                                    break;
                                                case MACROEXPANSION:
                                                {
                                                    std::string MacroExpansion;
                                                    auto MacroDefinition = CurrentTable->Macros.find(Mnemonic);
                                                    if(MacroDefinition != CurrentTable->Macros.end())
                                                        ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                    else if(CurrentTable != &MainTable)
                                                    {
                                                        MacroDefinition = MainTable.Macros.find(Mnemonic);
                                                        if(MacroDefinition != MainTable.Macros.end())
                                                            ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                    }
                                                    if(!Source.InMacro())
                                                        LineNumber++;
                                                    if(!MacroExpansion.empty())
                                                        Source.InsertMacro(Mnemonic, MacroExpansion);
                                                    else
                                                        throw AssemblyException("Unknown OpCode", SEVERITY_Error);
                                                    break;
                                                }
                                                case DB:
                                                {
                                                    for(auto& Operand : Operands)
                                                    {
                                                        if(Operand[0] == '\"')
                                                        {
                                                            std::vector<std::uint8_t> Data;
                                                            StringToByteVector(Operand, Data);
                                                            SubroutineSize += Data.size();
                                                        }
                                                        else
                                                            SubroutineSize++;
                                                    }
                                                    break;
                                                }
                                                case DW:
                                                {
                                                    SubroutineSize += Operands.size() * 2;
                                                    break;
                                                }
                                                case DL:
                                                {
                                                    SubroutineSize += Operands.size() * 4;
                                                    break;
                                                }
                                                case END:
                                                    if(InSub)
                                                        throw AssemblyException("END cannot appear inside a SUBROUTINE", SEVERITY_Error);
                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("END requires a single argument <entry point>", SEVERITY_Error);
                                                    while(Source.getLine(OriginalLine))
                                                        ;
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }
                                        else
                                        {
                                            if(OpCode.value().CPUType > Processor)
                                                throw AssemblyException("Instruction not supported on selected processor", SEVERITY_Error);
                                            SubroutineSize += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                        }
                                    }
                                    break;
                                }
                                case 2: // Generate Symbol Tables
                                {
                                    if(!Label.empty() && UnReferencedSubs.count(Label)==0 && (!OpCode.has_value() || OpCode.value().OpCode != MACRO))
                                    {
                                        if(CurrentTable->Symbols.find(Label) == CurrentTable->Symbols.end())
                                            CurrentTable->Symbols[Label].Value = ProgramCounter;
                                        else
                                        {
                                            auto& Symbol = CurrentTable->Symbols[Label];
                                            if(Symbol.Value.has_value())
                                                throw AssemblyException(fmt::format("Label '{Label}' is already defined", fmt::arg("Label", Label)), SEVERITY_Error);
                                            Symbol.Value = ProgramCounter;
                                        }
                                    }

                                    if(OpCode)
                                    {
                                        if(OpCode.value().OpCodeType == PSEUDO_OP)
                                        {
                                            switch(OpCode.value().OpCode)
                                            {
                                                case EQU:
                                                {
                                                    if(Label.empty())
                                                        throw AssemblyException("EQU requires a Label", SEVERITY_Error);
                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("EQU Requires a single argument <value>", SEVERITY_Error);

                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Value = E.Evaluate(Operands[0]);
                                                        CurrentTable->Symbols[Label].Value = Value;
                                                    }
                                                    catch (ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                    }
                                                    break;
                                                }
                                                case SUB:
                                                {
                                                    if(UnReferencedSubs.count(Label) > 0) // Skip assembly if previously flagged as unreferenced and non-static
                                                    {
                                                        while(Source.getLine(OriginalLine))
                                                        {
                                                            LineNumber++;
                                                            std::string Line = Trim(OriginalLine);
                                                            if(Line.size()>0)
                                                            {
                                                                // Check for Pre-Processor Control statement (#control expression...)
                                                                std::smatch MatchResult;
                                                                if(regex_match(Line, MatchResult, std::regex(R"-(^#(\w+)(\s+(.*))?$)-")))
                                                                {
                                                                    std::string ControlWord = MatchResult[1];
                                                                    std::string Expression = MatchResult[3];
                                                                    if(PreProcessorControlLookup.find(ControlWord) != PreProcessorControlLookup.end())
                                                                    {
                                                                        switch(PreProcessorControlLookup.at(ControlWord))
                                                                        {
                                                                            case PP_LINE:
                                                                            {
                                                                                std::smatch MatchResult;
                                                                                std::string NewFile;
                                                                                if(regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-")))
                                                                                {
                                                                                    CurrentFile = MatchResult[1];
                                                                                    LineNumber = stoi(MatchResult[2]);
                                                                                }
                                                                                else
                                                                                    throw AssemblyException("Bad line directive received from Pre-Processor", SEVERITY_Error);
                                                                                break;
                                                                            }
                                                                            default:
                                                                                break;
                                                                        }
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    std::string Label;
                                                                    std::string Mnemonic;
                                                                    std::vector<std::string>Operands;
                                                                    std::optional<OpCodeSpec> OpCode = ExpandTokens(Line, Label, Mnemonic, Operands);
                                                                    if(OpCode.has_value() && OpCode.value().OpCode == ENDSUB)
                                                                        break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    else // Assemble subroutine
                                                    {
                                                        CurrentTable = &SubTables[Label];
                                                        CurrentTable->Name = Label;
                                                        SubDefinitionFile = CurrentFile;
                                                        for(int i = 0; i < Operands.size(); i++)
                                                        {
                                                            std::vector<std::string> SubOptions;
                                                            StringListToVector(Operands[i], SubOptions, '=');
                                                            ToUpper(SubOptions[0]);
                                                            auto Option = SubroutineOptionsLookup.find(SubOptions[0]);
                                                            if(Option == SubroutineOptionsLookup.end())
                                                                throw AssemblyException("Unrecognised SUBROUTINE option", SEVERITY_Warning);
                                                            switch(Option->second)
                                                            {
                                                                case SUBOPT_ALIGN:
                                                                    if(SubOptions.size() != 2)
                                                                        throw AssemblyException("Unrecognised SUBROUTINE ALIGN option", SEVERITY_Error);
                                                                    else
                                                                    {
                                                                        int Align;
                                                                        if(SubOptions[1] == "AUTO")
                                                                        {
                                                                            Align = AlignFromSize(CurrentTable->CodeSize);
                                                                            InAutoAlignedSub = true;
                                                                        }
                                                                        else if(!SetAlignFromKeyword(SubOptions[1], Align))
                                                                        {
                                                                            try
                                                                            {
                                                                                AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                                                Align = E.Evaluate(SubOptions[1]);
                                                                                if(Align != 2 && Align != 4 && Align != 8 && Align != 16 && Align != 32 && Align != 64 && Align != 129 && Align !=256)
                                                                                    throw AssemblyException("SUBROUTINE ALIGN must be 2,4,8,16,32,64,128,256 or AUTO", SEVERITY_Error);
                                                                            }
                                                                            catch (ExpressionException Ex)
                                                                            {
                                                                                throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                                            }
                                                                        }
                                                                        ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                                                        MainTable.Symbols[Label].Value = ProgramCounter;
                                                                    }
                                                                    break;

                                                                case SUBOPT_STATIC:
                                                                    if(SubOptions.size() != 1)
                                                                        throw AssemblyException("SUBROUTINE STATIC option does not take any arguments", SEVERITY_Error);
                                                                    CurrentTable->Static = true;
                                                                    break;

                                                                default:
                                                                    throw AssemblyException("Unrecognised SUBROUTINE option", SEVERITY_Warning);
                                                                    break;
                                                            }
                                                        }
                                                        CurrentTable->Symbols[Label].Value = ProgramCounter;
                                                        break;
                                                    }
                                                    break;
                                                }
                                                case ENDSUB:
                                                {
                                                    InAutoAlignedSub = false;
                                                    switch(Operands.size())
                                                    {
                                                        case 0:
                                                            break;
                                                        case 1:
                                                            try
                                                            {
                                                                AssemblyExpressionEvaluator E(*CurrentTable, ProgramCounter, Processor);
                                                                auto EntryPoint = E.Evaluate(Operands[0]);
                                                                MainTable.Symbols[CurrentTable->Name].Value = EntryPoint;
                                                                CurrentTable->Symbols[CurrentTable->Name].Value = EntryPoint;
                                                                break;
                                                            }
                                                            catch (ExpressionException Ex)
                                                            {
                                                                throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                            }

                                                        default:
                                                            throw AssemblyException("Incorrect number of arguments", SEVERITY_Error);
                                                    }
                                                    CurrentTable = &MainTable;
                                                    break;
                                                }
                                                case MACRO:
                                                {
                                                    while(Source.getLine(OriginalLine))
                                                    {
                                                        LineNumber++;
                                                        std::string Line = Trim(OriginalLine);
                                                        try
                                                        {
                                                            OpCode = ExpandTokens(Line, Label, Mnemonic, Operands);
                                                        }
                                                        catch(AssemblyException Ex)
                                                        {
                                                            Label = {};
                                                            Mnemonic = {};
                                                            OpCode = {};
                                                            Operands = {};
                                                        }
                                                        if(OpCode.has_value() && OpCode.value().OpCode == ENDMACRO)
                                                            break;
                                                    }
                                                    break;
                                                }
                                                case MACROEXPANSION:
                                                {
                                                    std::string MacroExpansion;
                                                    auto MacroDefinition = CurrentTable->Macros.find(Mnemonic);
                                                    if(MacroDefinition != CurrentTable->Macros.end())
                                                        ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                    else if(CurrentTable != &MainTable)
                                                    {
                                                        MacroDefinition = MainTable.Macros.find(Mnemonic);
                                                        if(MacroDefinition != MainTable.Macros.end())
                                                            ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                    }
                                                    if(!Source.InMacro())
                                                        LineNumber++;
                                                    if(!MacroExpansion.empty())
                                                        Source.InsertMacro(Mnemonic, MacroExpansion);
                                                    else
                                                        throw AssemblyException("Unknown OpCode", SEVERITY_Error);
                                                    break;
                                                }
                                                case ORG:
                                                {
                                                    if(CurrentTable != &MainTable)
                                                        throw AssemblyException("ORG Cannot be used in a SUBROUTINE", SEVERITY_Error);

                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("ORG Requires a single argument <address>", SEVERITY_Error);

                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        int x = E.Evaluate(Operands[0]);
                                                        if(x >= 0 && x < 0x10000)
                                                            ProgramCounter = x;
                                                        else
                                                            throw AssemblyException("Overflow: Address must be in range 0-FFFF", SEVERITY_Error);
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                    }
                                                    if(!Label.empty())
                                                        CurrentTable->Symbols[Label].Value = ProgramCounter;

                                                    break;
                                                }
                                                case DB:
                                                {
                                                    for(auto& Operand : Operands)
                                                    {
                                                        if(Operand[0] == '\"')
                                                        {
                                                            std::vector<std::uint8_t> Data;
                                                            StringToByteVector(Operand, Data);
                                                            ProgramCounter += Data.size();
                                                        }
                                                        else
                                                            ProgramCounter++;
                                                    }
                                                    break;
                                                }
                                                case DW:
                                                {
                                                    ProgramCounter += Operands.size() * 2;
                                                    break;
                                                }
                                                case DL:
                                                {
                                                    ProgramCounter += Operands.size() * 4;
                                                    break;
                                                }
                                                case ALIGN:
                                                {
                                                    if(InAutoAlignedSub)
                                                        throw AssemblyException("ALIGN cannot be used inside an AUTO Aligned SUBROUTINE", SEVERITY_Error);
                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("ALIGN Requires a single argument <alignment>", SEVERITY_Error);
                                                    try
                                                    {
                                                        int Align;
                                                        if(!SetAlignFromKeyword(Operands[0], Align))
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Align = E.Evaluate(Operands[0]);
                                                            if(Align != 2 && Align != 4 && Align != 8 && Align != 16 && Align != 32 && Align != 64 && Align != 129 && Align !=256)
                                                                throw AssemblyException("ALIGN must be 2,4,8,16,32,64,128 or 256", SEVERITY_Error);
                                                        }
                                                        ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                    }

                                                    if(!Label.empty())
                                                        CurrentTable->Symbols[Label].Value = ProgramCounter;
                                                    break;
                                                }
                                                case END:
                                                    while(Source.getLine(OriginalLine))
                                                        ;
                                                default:
                                                    break;
                                            }
                                        }
                                        else if(OpCode && OpCode.value().OpCodeType != PSEUDO_OP)
                                            ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                    }
                                    break;
                                }
                                case 3: // Generate Code
                                {
                                    if(OpCode)
                                    {
                                        if(OpCode.value().OpCodeType == PSEUDO_OP)
                                        {
                                            switch(OpCode.value().OpCode)
                                            {
                                                case EQU:
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    break;
                                                case SUB:
                                                {
                                                    if(UnReferencedSubs.count(Label) > 0) // Skip assembly if previously flagged as unreferenced and non-static
                                                    {
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                        while(Source.getLine(OriginalLine))
                                                        {
                                                            LineNumber++;
                                                            std::string Line = Trim(OriginalLine);
                                                            if(Line.size()>0)
                                                            {
                                                                // Check for Pre-Processor Control statement (#control expression...)
                                                                std::smatch MatchResult;
                                                                if(regex_match(Line, MatchResult, std::regex(R"-(^#(\w+)(\s+(.*))?$)-")))
                                                                {
                                                                    std::string ControlWord = MatchResult[1];
                                                                    std::string Expression = MatchResult[3];
                                                                    if(PreProcessorControlLookup.find(ControlWord) != PreProcessorControlLookup.end())
                                                                    {
                                                                        switch(PreProcessorControlLookup.at(ControlWord))
                                                                        {
                                                                            case PP_LINE:
                                                                            {
                                                                                std::smatch MatchResult;
                                                                                std::string NewFile;
                                                                                if(regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-")))
                                                                                {
                                                                                    CurrentFile = MatchResult[1];
                                                                                    LineNumber = stoi(MatchResult[2]) - 1;
                                                                                }
                                                                                else
                                                                                    throw AssemblyException("Bad line directive received from Pre-Processor", SEVERITY_Error);
                                                                                break;
                                                                            }
                                                                            default:
                                                                                ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                                                break;
                                                                        }
                                                                    }
                                                                    else
                                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                                }
                                                                else
                                                                {
                                                                    std::string Label;
                                                                    std::string Mnemonic;
                                                                    std::vector<std::string>Operands;
                                                                    std::optional<OpCodeSpec> OpCode = ExpandTokens(Line, Label, Mnemonic, Operands);
                                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                                    if(OpCode.has_value() && OpCode.value().OpCode == ENDSUB)
                                                                        break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    else // Assemble subroutine
                                                    {
                                                        CurrentTable = &SubTables[Label];
                                                        SubDefinitionFile = CurrentFile;
                                                        for(int i = 0; i < Operands.size(); i++)
                                                        {
                                                            std::vector<std::string> SubOptions;
                                                            StringListToVector(Operands[i], SubOptions, '=');
                                                            ToUpper(SubOptions[0]);
                                                            auto Option = SubroutineOptionsLookup.find(SubOptions[0]);
                                                            if(Option == SubroutineOptionsLookup.end())
                                                                throw AssemblyException("Unrecognised SUBROUTINE option", SEVERITY_Warning);
                                                            switch(Option->second)
                                                            {
                                                                case SUBOPT_ALIGN:
                                                                    try
                                                                    {
                                                                        int Align;
                                                                        if(SubOptions[1] == "AUTO")
                                                                            Align = AlignFromSize(CurrentTable->CodeSize);
                                                                        else if(!SetAlignFromKeyword(SubOptions[1], Align))
                                                                        {
                                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                                            Align = E.Evaluate(SubOptions[1]);
                                                                        }
                                                                        ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                                                        CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                                    }
                                                                    catch(ExpressionException Ex)
                                                                    {
                                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                                    }
                                                                    break;
                                                                case SUBOPT_STATIC:
                                                                    break;
                                                            }
                                                        }
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    }
                                                    break;
                                                }
                                                case ENDSUB:
                                                {
                                                    CurrentTable = &MainTable;
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    break;
                                                }
                                                case MACRO:
                                                {
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    while(Source.getLine(OriginalLine))
                                                    {
                                                        LineNumber++;
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                        std::string Line = Trim(OriginalLine);
                                                        try
                                                        {
                                                            OpCode = ExpandTokens(Line, Label, Mnemonic, Operands);
                                                        }
                                                        catch(AssemblyException Ex)
                                                        {
                                                            Label = {};
                                                            Mnemonic = {};
                                                            OpCode = {};
                                                            Operands = {};
                                                        }
                                                        if(OpCode.has_value() && OpCode.value().OpCode == ENDMACRO)
                                                            break;
                                                    }
                                                    break;
                                                }
                                                case MACROEXPANSION:
                                                {
                                                    std::string MacroExpansion;
                                                    auto MacroDefinition = CurrentTable->Macros.find(Mnemonic);
                                                    if(MacroDefinition != CurrentTable->Macros.end())
                                                        ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                    else if(CurrentTable != &MainTable)
                                                    {
                                                        MacroDefinition = MainTable.Macros.find(Mnemonic);
                                                        if(MacroDefinition != MainTable.Macros.end())
                                                            ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                    }
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    if(!Source.InMacro())
                                                        LineNumber++;
                                                    if(!MacroExpansion.empty())
                                                        Source.InsertMacro(Mnemonic, MacroExpansion);
                                                    else
                                                        throw AssemblyException("Unknown OpCode", SEVERITY_Error);
                                                    break;
                                                }
                                                case ORG:
                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        ProgramCounter = E.Evaluate(Operands[0]);

                                                        CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;

                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                        break;
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                    }

                                                case DB:
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                    if(CurrentTable != &MainTable)
                                                        E.AddLocalSymbols(CurrentTable);
                                                    for(auto& Operand : Operands)
                                                    {
                                                        if(Operand[0] == '\"')
                                                            StringToByteVector(Operand, Data);
                                                        else
                                                            try
                                                            {
                                                                int x = E.Evaluate(Operand);
                                                                if(x > 255)
                                                                    throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FF, got: ${value:X})", fmt::arg("value", x)), SEVERITY_Error);
                                                                Data.push_back(x & 0xFF);
                                                            }
                                                            catch(ExpressionException Ex)
                                                            {
                                                                throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                            }
                                                    }
                                                    CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                    ProgramCounter += Data.size();
                                                    break;
                                                }
                                                case DW:
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                    if(CurrentTable != &MainTable)
                                                        E.AddLocalSymbols(CurrentTable);
                                                    for(auto& Operand : Operands)
                                                        try
                                                        {
                                                            int x = E.Evaluate(Operand);
                                                            Data.push_back((x >> 8) & 0xFF);
                                                            Data.push_back(x & 0xFF);
                                                        }
                                                        catch(ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                        }
                                                    CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                    ProgramCounter += Data.size();
                                                    break;
                                                }
                                                case DL:
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                    if(CurrentTable != &MainTable)
                                                        E.AddLocalSymbols(CurrentTable);
                                                    for(auto& Operand : Operands)
                                                        try
                                                        {
                                                            int x = E.Evaluate(Operand);
                                                            Data.push_back((x >> 24) & 0xFF);
                                                            Data.push_back((x >> 16) & 0xFF);
                                                            Data.push_back((x >> 8) & 0xFF);
                                                            Data.push_back(x & 0xFF);
                                                        }
                                                        catch(ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                        }
                                                    CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                    ProgramCounter += Data.size();
                                                    break;
                                                }
                                                case ALIGN:
                                                    try
                                                    {
                                                        int Align;
                                                        if(!SetAlignFromKeyword(Operands[0], Align))
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Align = E.Evaluate(Operands[0]);
                                                        }
                                                        ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                                        CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                        break;
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                    }
                                                case ASSERT:
                                                {
                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("ASSERT Requires a single argument <expression>", SEVERITY_Error);
                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Result = E.Evaluate(Operands[0]);
                                                        if (Result == 0)
                                                            throw AssemblyException("ASSERT Failed", SEVERITY_Error);
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                    }
                                                    break;
                                                }
                                                case END:
                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        EntryPoint = E.Evaluate(Operands[0]);
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                        while(Source.getLine(OriginalLine))
                                                            ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), SEVERITY_Error);
                                                    }
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }
                                        else
                                            try
                                            {
                                                std::vector<std::uint8_t> Data;
                                                AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                if(CurrentTable != &MainTable)
                                                    E.AddLocalSymbols(CurrentTable);

                                                switch(OpCode->OpCodeType)
                                                {
                                                    case BASIC:
                                                    {
                                                        Data.push_back(OpCode->OpCode);
                                                        break;
                                                    }
                                                    case REGISTER:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Register", SEVERITY_Error, OpCode->OpCodeType);

                                                        int Register = E.Evaluate(Operands[0]);
                                                        if(OpCode->OpCode == LDN) // Special Case - LDN R0 is overriden by IDL
                                                        {
                                                            if(Register < 1 || Register > 15)
                                                                throw AssemblyException(fmt::format("Register out of range (Expected: $1-$F, got: ${value:X}))", fmt::arg("value", Register)), SEVERITY_Error, OpCode->OpCodeType);
                                                        }
                                                        else
                                                        {
                                                            if(Register < 0 || Register > 15)
                                                                throw AssemblyException(fmt::format("Register out of range (Expected: $0-$F, got: ${value:X}))", fmt::arg("value", Register)), SEVERITY_Error, OpCode->OpCodeType);
                                                        }
                                                        Data.push_back(OpCode->OpCode | Register);
                                                        break;
                                                    }
                                                    case IMMEDIATE:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Byte", SEVERITY_Error, OpCode->OpCodeType);
                                                        int Byte = E.Evaluate(Operands[0]);
                                                        if(Byte > 0xFF && Byte < 0xFF80)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FF, got: ${value:X})", fmt::arg("value", Byte)), SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Byte & 0xFF);
                                                        break;
                                                    }
                                                    case SHORT_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Short Branch expected single operand", SEVERITY_Error, OpCode->OpCodeType);
                                                        int Address = E.Evaluate(Operands[0]);
                                                        if(((ProgramCounter + 1) & 0xFF00) != (Address & 0xFF00))
                                                            throw AssemblyException("Short Branch out of range", SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case LONG_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Long Branch expected single operand", SEVERITY_Error, OpCode->OpCodeType);
                                                        int Address = E.Evaluate(Operands[0]);
                                                        if(Address < 0 || Address > 0xFFFF)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FFFF, got: ${value:X})", fmt::arg("value", Address)), SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Address >> 8);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case INPUT_OUTPUT:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Port", SEVERITY_Error, OpCode->OpCodeType);

                                                        int Port = E.Evaluate(Operands[0]);
                                                        if(Port == 0 || Port > 7)
                                                            throw AssemblyException("Port out of range (1-7)", SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode | Port);
                                                        break;
                                                    }
                                                    case EXTENDED:
                                                    {
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        break;
                                                    }
                                                    case EXTENDED_REGISTER:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Register", SEVERITY_Error, OpCode->OpCodeType);

                                                        int Register = E.Evaluate(Operands[0]);
                                                        if(Register < 0 || Register > 15)
                                                            throw AssemblyException("Register out of range (0-F)", SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF | Register);
                                                        break;
                                                    }
                                                    case EXTENDED_IMMEDIATE:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Byte", SEVERITY_Error, OpCode->OpCodeType);
                                                        int Byte = E.Evaluate(Operands[0]);
                                                        if(Byte > 0xFF && Byte < 0xFF80)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FF, got :${value:X})", fmt::arg("value", Byte)), SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        Data.push_back(Byte & 0xFF);
                                                        break;
                                                    }
                                                    case EXTENDED_SHORT_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Short Branch expected single operand", SEVERITY_Error, OpCode->OpCodeType);
                                                        int Address = E.Evaluate(Operands[0]);
                                                        if(((ProgramCounter + 2) & 0xFF00) != (Address & 0xFF00))
                                                            throw AssemblyException("Short Branch out of range", SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case EXTENDED_REGISTER_IMMEDIATE16:
                                                    {
                                                        if(Operands.size() != 2)
                                                            throw AssemblyException("Expected Register and Immediate operands", SEVERITY_Error, OpCode->OpCodeType);
                                                        int Register = E.Evaluate(Operands[0]);
                                                        if(Register > 15)
                                                            throw AssemblyException("Register out of range (0-F)", SEVERITY_Error, OpCode->OpCodeType);
                                                        int Address = E.Evaluate(Operands[1]);
                                                        if(Address < -32768 || Address > 0xFFFF)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FFFF, got :${value:X})", fmt::arg("value", Address)), SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF | Register);
                                                        Data.push_back(Address >> 8);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    default:
                                                        break;
                                                }

                                                CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());

                                                ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                            }
                                            catch(ExpressionException Ex)
                                            {
                                                throw AssemblyException(Ex.what(), SEVERITY_Error, OpCode->OpCodeType);
                                            }
                                    }
                                    else
                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                }
                            }
                        }
                        catch (AssemblyException Ex)
                        {
                            if(!Errors.Contains(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), Ex.what(), Ex.Severity, Source.InMacro()))
                            {
                                PrintError(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), Line, Ex.what(), Ex.Severity, Source.InMacro());
                                Errors.Push(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), Line, Ex.what(), Ex.Severity, Source.InMacro());
                            }
                            if(Ex.SkipToOpCode.has_value())
                            {
                                while(Source.getLine(OriginalLine))
                                {
                                    std::string Line = Trim(OriginalLine);
                                    std::string Label;
                                    std::string Mnemonic;
                                    std::vector<std::string>Operands;
                                    std::optional<OpCodeSpec> OpCode = ExpandTokens(Line, Label, Mnemonic, Operands);
                                    if(OpCode.has_value() && OpCode.value().OpCode == Ex.SkipToOpCode)
                                        break;
                                }
                            }
                            if (Pass == 3)
                            {
                                if(Ex.BytesToSkip == 0)
                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                else
                                {
                                    std::vector<std::uint8_t> Data;
                                    for(int i = 0; i< Ex.BytesToSkip; i++)
                                        Data.push_back(0);
                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                }
                            }
                            ProgramCounter += Ex.BytesToSkip;
                        }
                    }
                    else // Empty line
                        if (Pass == 3)
                            ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                }
                if(!Source.InMacro())
                    LineNumber++;
            } // while(Source.getLine())...

            // Custom processing at the end of each pass
            switch(Pass)
            {
                case 1:
                    // Verify #if nesting structure
                    if(IfNestingLevel != 0)
                        throw AssemblyException("#if Nesting Error or missing #endif", SEVERITY_Warning);
                    break;
                case 2:
                    break;
                case 3:
                {
                    // Check for END statement
                    if(!EntryPoint.has_value())
                        throw AssemblyException("END Statement is missing", SEVERITY_Warning);

                    // Check for un-used non-static SUBROUTINEs and reset to Pass 2 if found
                    for(const auto& SubTable : SubTables)
                        if(MainTable.Symbols[SubTable.first].RefCount == 0 && ! SubTable.second.Static)
                        {
                            UnReferencedSubs.insert(SubTable.first);
                            Pass = 1;
                        }
                    if(Pass == 1)
                    {
                        fmt::println("Unreferenced SUBROUTINES found, Removing...");
                        for(auto& Name : UnReferencedSubs)
                            fmt::println("\t{Name}",fmt::arg("Name", Name));
                        fmt::println("Restarting from Pass 2");

                        // Clear Master Symbol Table (Except hidden symbols, i.e. R0-F & P1-7)
                        for(auto it = MainTable.Symbols.begin(); it != MainTable.Symbols.end(); )
                        {
                            if(!it->second.HideFromSymbolTable)
                                it = MainTable.Symbols.erase(it);
                            else
                                ++it;
                        }

                        // Clear Sub Symbol Tables
                        SubTables.clear();

                        // Reset Listing File
                        ListingFile.Reset();
                        break;
                    }

                    // Check for overlapping code
                    int Overlap = 0;
                    for(auto &Code1 : Code)
                    {
                        uint16_t Start1 = Code1.first;

                        for(auto &Code2 : Code)
                        {
                            uint16_t Start2 = Code2.first;
                            uint16_t End2 = Code2.first + Code2.second.size();
                            if (Start1 >= Start2 && Start1 < End2)
                                Overlap++;
                        }
                    }
                    if(Overlap > Code.size())
                        throw AssemblyException("Code blocks overlap", SEVERITY_Warning);
                    break;
                }
            }
        }
        catch (AssemblyException Ex)
        {
            PrintError(Ex.what(), Ex.Severity);
            Errors.Push(Ex.what(), Ex.Severity);
            if(Ex.SkipToOpCode.has_value())
                throw; // AssemblyException is only thrown with a SkipToOpcode in an enclosed try / catch so this should never happen
        }

    } // for(int Pass = 1; Pass <= 3 && Errors.count(SEVERITY_Error) == 0; Pass++)...

    ListingFile.AppendGlobalErrors();

    if(DumpSymbols)
    {
        ListingFile.AppendSymbols("Global Symbols", MainTable);
        for(auto& Table : SubTables)
            ListingFile.AppendSymbols(Table.first, Table.second);
    }

    int TotalWarnings = Errors.count(SEVERITY_Warning);
    int TotalErrors = Errors.count(SEVERITY_Error);

    fmt::println("");
    fmt::println("{count:4} Warnings",     fmt::arg("count", TotalWarnings));
    fmt::println("{count:4} Errors",       fmt::arg("count", TotalErrors));
    fmt::println("");

    // If no Errors, then write the binary output
    if(TotalErrors == 0)
    {
        BinaryWriter *Output;
        switch(BinMode)
        {
            case INTEL_HEX:
            {
                Output = new BinaryWriter_IntelHex(FileName, "hex");
                break;
            }
            case IDIOT4:
            {
                Output = new BinaryWriter_Idiot4(FileName, "idiot");
                break;
            }
        }
        Output->Write(Code, EntryPoint);
        delete Output;
    }

    return TotalErrors == 0 && TotalWarnings == 0;
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

    bool inSingleQuotes = false;
    bool inDoubleQuotes = false;
    bool inEscape = false;

    for(int i = 0; i < Input.size(); i++)
    {
        char ch = Input[i];

        if(inEscape)
        {
            Output += ch;
            inEscape = false;
        }
        else if(inSingleQuotes)
        {
            Output += ch;

            if(ch == '\\')
                inEscape = true;

            if(inSingleQuotes && ch == '\'' && !inEscape)
                inSingleQuotes = false;
        }
        else if(inDoubleQuotes)
        {
            Output += ch;

            if(ch == '\\')
                inEscape = true;

            if(inDoubleQuotes && ch == '\"' && !inEscape)
                inDoubleQuotes = false;
        }
        else if(ch == '\'')
        {
            inSingleQuotes = true;
            Output += ch;
        }
        else if(ch == '\"')
        {
            inDoubleQuotes = true;
            Output += ch;
        }
        else
        {
            std::string Identifier;
            int j = i;
            while(isalnum(Input[j]) || Input[j]=='_')
                Identifier += Input[j++];
            if(Identifier.size() > 0)
            {
                std::string UCIdentifier(Identifier);
                ToUpper(UCIdentifier);
                if(Parameters.find(UCIdentifier) != Parameters.end())
                    Output += Parameters[UCIdentifier];
                else
                    Output += Identifier;
                i += Identifier.size() - 1;
            }
            else
                Output += ch;
        }
    }
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

//!
//! \brief StringListToVector
//! \param Input
//! \param Output
//! \param Delimiter
//!
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
//! \brief PrintError
//! \param SourceFileName
//! \param LineNumber
//! \param Line
//! \param Message
//! \param Severity
//!
//! Print the error to StdErr
//!
void PrintError(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const std::string& Message, const AssemblyErrorSeverity Severity, const bool InMacro)
{
    std::string FileRef;
    std::string LineRef;
    if(InMacro)
    {
        FileRef = FileName+"::"+MacroName;
        LineRef = fmt::format("{LineNumber:05}.{MacroLineNumber:02}", fmt::arg("LineNumber", LineNumber-1), fmt::arg("MacroLineNumber", MacroLineNumber));
    }
    else
    {
        FileRef = FileName;
        LineRef = fmt::format("{LineNumber:05}   ", fmt::arg("LineNumber", LineNumber));
    }

    try // Source may not contain anything...
    {
        fmt::println("[{filename:21.21}{linenumber:>8}] {line}",
                     fmt::arg("filename", FileRef),
                     fmt::arg("linenumber", LineRef),
                     fmt::arg("line", Line)
                    );
        fmt::println("***************{severity:*>15}: {message}",
                     fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                     fmt::arg("message", Message));
    }
    catch(...)
    {
        fmt::println("***************{severity:*>15}: {message}",
                     fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                     fmt::arg("message", Message));
    }
}

void PrintError(const std::string& Message, AssemblyErrorSeverity Severity)
{
    fmt::println("***************{severity:*>15}: {message}",
                 fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                 fmt::arg("message", Message));
}

bool SetAlignFromKeyword(std::string Alignment, int& Align)
{
    static std::map<std::string, int> Lookup =
    {
        { "WORD",   2 },
        { "DWORD",  4 },
        { "QWORD",  8 },
        { "PARA",  16 },
        { "PAGE", 256 }
    };
    ToUpper(Alignment);
    auto i = Lookup.find(Alignment);
    if(i == Lookup.end())
        return false;
    else
    {
        Align = i->second;
        return true;
    }
}
