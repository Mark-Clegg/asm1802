#include <cstring>
#include <filesystem>
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
#include "definemap.h"
#include "errortable.h"
#include "assemblyexception.h"
#include "expressionevaluator.h"
#include "listingfilewriter.h"
#include "opcodetable.h"
#include "sourcecodereader.h"
#include "symboltable.h"
#include "utils.h"

namespace fs = std::filesystem;

std::string Version("0.1");

enum SubroutineOptionsEnum
{
    SUBOPT_ALIGN
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

std::map<std::string, SubroutineOptionsEnum> SubroutineOptionsLookup =
{
    { "ALIGN",      SUBOPT_ALIGN      }
};

DefineMap GlobalDefines;   // global #defines persist for all files following on the command line

bool assemble(const std::string&, bool ListingEnabled, bool DumpSymbols, OutputFormatEnum BinMode);
void PrintError(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity);
void PrintError(const std::string& Message, AssemblyErrorSeverity Severity);
void PrintSymbols(const std::string & Name, const SymbolTable& Table);
bool NoRegisters = false;   // Suppress pre-defined Register equates
bool NoPorts     = false;   // Suppress pre-defined Port equates

#if DEBUG
void DumpCode(const std::map<uint16_t, std::vector<uint8_t>>& Code);
#endif

int main(int argc, char **argv)
{
    option longopts[] =
    {
        { "define",      required_argument, 0, 'D' }, // Define pre-processor variable
        { "undefine",    required_argument, 0, 'U' }, // Un-define pre-processor variable
        { "list",        no_argument,       0, 'l' }, // Create a listing file after pass 3
        { "symbols",     no_argument,       0, 's' }, // Include Symbol Table in listing file
        { "noregisters", no_argument,       0, 'r' }, // Do not pre-define labels for Registers (R0-F, R0-15)
        { "noports",     no_argument,       0, 'p' }, // No not pre-define labels for Ports (P1-7)
        { "output",      required_argument, 0, 'o' }, // Set output file type (default = Intel Hex)
        { "version",     no_argument,       0, 'v' }, // Print version number and exit
        { "help",        no_argument,       0, '?' }, // Print using information
        { 0,0,0,0 }
    };

    bool Listing = false;
    bool Symbols = false;
    OutputFormatEnum OutputFormat = INTEL_HEX;
    int FileCount = 0;
    int FilesAssembled = 0;
    while (1)
    {
        const int opt = getopt_long(argc, argv, "D:U:lso:v?", longopts, 0);

        if (opt == -1)
            break;

        switch (opt)
        {
            case 'D':
            {
            }
            default:
                break;
        }

        switch (opt)
        {
            case 'D':
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
                ToUpper(value);
                if(GlobalDefines.contains(key))
                    fmt::print("** Warning: Macro redeclared: {key}\n", fmt::arg("key", key));
                GlobalDefines[key] = value;
                break;
            }

            case 'U':
            {
                std::string key = optarg;
                ToUpper(key);
                GlobalDefines.erase(optarg);
                break;
            }
            case 'l':
                Listing = true;
                break;

            case 's':
                Symbols = true;
                break;

            case 'r':
                NoRegisters = true;
                break;

            case 'p':
                NoPorts = true;
                break;

            case 'o':
            {
                std::string Mode = optarg;
                ToUpper(Mode);
                if(OutputFormatLookup.find(Mode) == OutputFormatLookup.end())
                    fmt::print("** Unrecognised binary output mode. Defaulting to Intel Hex\n");
                else
                    OutputFormat = OutputFormatLookup.at(Mode);
                break;
            }
            case 'v':
            {
                fmt::print("{version}\n", fmt::arg("version", Version));
                return 0;
            }
            case '?':
            {
                std::string FileName = fs::path(argv[0]).filename();
                fmt::println("{FileName}: Version {Version}", fmt::arg("FileName", FileName), fmt::arg("Version", Version));
                fmt::println("Macro Assembler for the COSMAC CDP1802 series MicroProcessor");
                fmt::println("Usage:");
                fmt::println("asm1802 <options> SourceFile <options>");
                fmt::println("");
                fmt::println("Options:");
                fmt::println("-D|--define Name{{=value}}");
                fmt::println("\tDefine preprocessor variable");
                fmt::println("-U|--undefine Name");
                fmt::println("\tUndefine preprocessor variable");
                fmt::println("-l|--list");
                fmt::println("\tCreate listing file");
                fmt::println("-s|--symbols");
                fmt::println("\tInclude Symbol Tables in listing");
                fmt::println("--noregisters");
                fmt::println("\tDo not predefine R0-RF register symbols");
                fmt::println("--noports");
                fmt::println("\tDo not predefine P1-P7 port symbols");
                fmt::println("-o|--output format");
                fmt::println("\tCreate output file in \"intelhex\" or \"idiiot4\" format");
                fmt::println("-v|--version");
                fmt::println("\tPrint version number and exit");
                fmt::println("-?|--help");
                fmt::println("\tPrint thie help and exit");
                return 0;
            }
            default:
            {
                fmt::print("Error\n");
                return 1;
            }
        }
    }

    while (optind < argc)
    {
        try
        {
            if(assemble(argv[optind++], Listing, Symbols, OutputFormat))
                FilesAssembled++;
            FileCount++;
        }
        catch (AssemblyException Error)
        {
            fmt::print("** Error opening/reading file: {message}\n", fmt::arg("message", Error.Message));
        }
    }

    fmt::print("{count:4} Files Assembled\n", fmt::arg("count", FileCount));
    fmt::print("{count:4} Files Failed\n",    fmt::arg("count", FileCount - FilesAssembled));

    if(FileCount == FilesAssembled)
        return 0;
    else
        return 1;
}

//!
//! \brief assemble
//! \param FileName
//! \return
//!
//! Main Assembler
//!
bool assemble(const std::string& FileName, bool ListingEnabled, bool DumpSymbols, OutputFormatEnum BinMode)
{
    fmt::print("Assembling: {filename}\n", fmt::arg("filename", FileName));

    SymbolTable MainTable;
    std::map<std::string, SymbolTable> SubTables;
    std::map<uint16_t, std::vector<uint8_t>> Code = {{ 0, {}}};
    std::map<uint16_t, std::vector<uint8_t>>::iterator CurrentCode = Code.begin();
    std::optional<uint16_t> EntryPoint;

    // Pre-Define DEFINES for common alignments
    GlobalDefines["WORD"]  = "2";
    GlobalDefines["DWORD"] = "4";
    GlobalDefines["QWORD"] = "8";
    GlobalDefines["PAGE"]  = "256";

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
    MainTable.Symbols["ON"]    = { 1, true };
    MainTable.Symbols["OFF"]   = { 0, true };
    MainTable.Symbols["TRUE"]  = { 1, true };
    MainTable.Symbols["FALSE"] = { 0, true };

    SourceCodeReader Source;
    ErrorTable Errors;
    ListingFileWriter ListingFile(Source, FileName, Errors, ListingEnabled);

    for(int Pass = 1; Pass <= 3 && Errors.count(SEVERITY_Error) == 0; Pass++)
    {
        SymbolTable* CurrentTable = &MainTable;
        uint16_t ProgramCounter = 0;
        uint16_t SubroutineSize = 0;
        CPUTypeEnum Processor = CPU_1802;
        bool InSub = false;
        bool InAutoAlignedSub = false;
        try
        {
            fmt::print("Pass {pass}\n", fmt::arg("pass", Pass));

            // Setup Source File stack
            Source.IncludeFile(FileName);

            // Initialise Defines from Global Defines
            DefineMap Defines(GlobalDefines);   // Defines, initialised from Global Defines

            // Setup stack of #if results
            int IfNestingLevel = 0;

            // Read and process each line
            std::string OriginalLine;
            while(Source.getLine(OriginalLine))
            {
                try
                {
                    std::string Line = trim(OriginalLine);
                    if (Line.size() > 0)
                    {
                        PreProcessorDirectiveEnum Directive;
                        std::string Expression;

                        if (IsPreProcessorDirective(Line, Directive, Expression)) // Pre-Processor Directive
                        {
                            if (Pass == 3)
                                ListingFile.Append();
                            switch (Directive)
                            {
                                case PP_define:
                                {
                                    //if(Source.getStreamType() == SourceCodeReader::SOURCE_LITERAL)
                                    //    throw AssemblyException("Cannot use #define inside a MACRO", SEVERITY_Error);
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
                                        value = "";
                                    }
                                    ToUpper(key);
                                    //if(Defines.contains(key))
                                    //    throw AssemblyException(fmt::format("Duplicate definition: {Name} is already defined.", fmt::arg("Name", key)), SEVERITY_Warning);
                                    Defines[key]=value;
                                    break;
                                }
                                case PP_undef:
                                {
                                    ToUpper(Expression);
                                    if(Defines.contains(Expression))
                                        Defines.erase(Expression);
                                    else
                                        throw AssemblyException("Not defined", SEVERITY_Warning);
                                    break;
                                }
                                case  PP_if:
                                {
                                    IfNestingLevel++;
                                    throw AssemblyException("Not Implemented - Assuming True", SEVERITY_Warning);
                                    break;
                                }
                                case PP_ifdef:
                                {
                                    IfNestingLevel++;
                                    ToUpper(Expression);
                                    if(!Defines.contains(Expression))
                                    {
                                        if(SkipLines(Source, OriginalLine) == PP_endif)
                                        {
                                            if (Pass == 3) // Add terminating directive from skiplines to listing output
                                                ListingFile.Append();
                                            IfNestingLevel--;
                                        }
                                    }
                                    break;
                                }
                                case PP_ifndef:
                                {
                                    IfNestingLevel++;
                                    ToUpper(Expression);
                                    if(Defines.contains(Expression))
                                    {
                                        if(SkipLines(Source, OriginalLine) == PP_endif)
                                        {
                                            if (Pass == 3) // Add terminating directive from skiplines to listing output
                                                ListingFile.Append();
                                            IfNestingLevel--;
                                        }
                                    }
                                    break;
                                }
                                case PP_else:
                                {
                                    if(SkipLines(Source, OriginalLine) == PP_endif)
                                    {
                                        if (Pass == 3) // Add terminating directive from skiplines to listing output
                                            ListingFile.Append();
                                        IfNestingLevel--;
                                    }
                                    break;
                                }
                                case PP_endif:
                                {
                                    IfNestingLevel--;
                                    break;
                                }
                                case PP_include:
                                {
                                    std::smatch MatchResult;
                                    if(regex_match(Expression, MatchResult, std::regex(R"(^[<\"]([^>\"]+)[>\"]$)")))
                                        Source.IncludeFile(MatchResult[1]);
                                    else
                                        throw AssemblyException("Unable to interpret filename expected <filename> or \"filename\"", SEVERITY_Error);
                                    break;
                                }
                            }
                        }
                        else // Assembly Source line
                        {
                            try
                            {
                                ExpandDefines(Line, Defines);

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
                                                            if(std::regex_match(Argument, ArgMatch))
                                                                if(std::find(MacroDefinition.Arguments.begin(), MacroDefinition.Arguments.end(), Argument) == MacroDefinition.Arguments.end())
                                                                    MacroDefinition.Arguments.push_back(Argument);
                                                                else
                                                                    throw AssemblyException("Macro arguments must be unique", SEVERITY_Error);
                                                            else
                                                                throw AssemblyException(fmt::format("Invalid argument name: '{Name}'", fmt::arg("Name", Argument)), SEVERITY_Error);
                                                        }

                                                        std::string Expansion;
                                                        while(Source.getLine(OriginalLine))
                                                        {
                                                            std::string Line = trim(OriginalLine);
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
                                                            Expansion.append(OriginalLine);
                                                            Expansion.append("\n");
                                                        }
                                                        MacroDefinition.Expansion = Expansion;
                                                        break;
                                                    }
                                                    case ENDMACRO:
                                                        throw AssemblyException("ENDMACRO without opening MACRO pseudo-op", SEVERITY_Error);
                                                        break;
                                                    case MACROEXPANSION:
                                                    {
                                                        auto MacroDefinition = CurrentTable->Macros.find(Mnemonic);
                                                        std::string MacroExpansion;
                                                        if(MacroDefinition != CurrentTable->Macros.end())
                                                            ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                        else if(CurrentTable != &MainTable)
                                                        {
                                                            MacroDefinition = MainTable.Macros.find(Mnemonic);
                                                            if(MacroDefinition != MainTable.Macros.end())
                                                                ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                        }
                                                        if(!MacroExpansion.empty())
                                                            Source.IncludeLiteral(Mnemonic, MacroExpansion);
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
                                                    case PROCESSOR:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("PROCESSOR requires a single argument", SEVERITY_Error);
                                                        if(OpCodeTable::CPUTable.find(Operands[0]) == OpCodeTable::CPUTable.end())
                                                            throw AssemblyException("Unknown Processor type specification", SEVERITY_Error);
                                                        Processor = OpCodeTable::CPUTable.at(Operands[0]);
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
                                        if(!Label.empty() && (!OpCode.has_value() || OpCode.value().OpCode != MACRO))
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

                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Value = E.Evaluate(Operands[0]);
                                                        CurrentTable->Symbols[Label].Value = Value;
                                                        break;
                                                    }
                                                    case SUB:
                                                    {
                                                        CurrentTable = &SubTables[Label];
                                                        CurrentTable->Name = Label;
                                                        for(int i = 0; i < Operands.size(); i++)
                                                        {
                                                            std::vector<std::string> SubOptions;
                                                            StringListToVector(Operands[i], SubOptions, '=');
                                                            auto Option = SubroutineOptionsLookup.find(SubOptions[0]);
                                                            if(Option == SubroutineOptionsLookup.end())
                                                                throw AssemblyException("Unrecognised SUBROUTINE option", SEVERITY_Warning);
                                                            switch(Option->second)
                                                            {
                                                                case SUBOPT_ALIGN:
                                                                    if(SubOptions.size() != 2)
                                                                        throw AssemblyException("Unrecognised SUBROUTINE ALIGN parameters", SEVERITY_Error);
                                                                    else
                                                                    {
                                                                        int Align;
                                                                        if(SubOptions[1] == "AUTO")
                                                                        {
                                                                            Align = AlignFromSize(CurrentTable->CodeSize);
                                                                            InAutoAlignedSub = true;
                                                                        }
                                                                        else
                                                                        {
                                                                            ExpressionEvaluator E(MainTable, ProgramCounter);
                                                                            Align = E.Evaluate(SubOptions[1]);
                                                                            if(Align != 2 && Align != 4 && Align != 8 && Align != 16 && Align != 32 && Align != 64 && Align != 129 && Align !=256)
                                                                                throw AssemblyException("SUBROUTINE ALIGN must be 2,4,8,16,32,64,128,256 or AUTO", SEVERITY_Error);
                                                                        }
                                                                        ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                                                        MainTable.Symbols[Label].Value = ProgramCounter;
                                                                    }
                                                                    break;
                                                                default:
                                                                    throw AssemblyException("Unrecognised SUBROUTINE option", SEVERITY_Warning);
                                                                    break;
                                                            }
                                                        }
                                                        CurrentTable->Symbols[Label].Value = ProgramCounter;
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
                                                            {
                                                                ExpressionEvaluator E(*CurrentTable, ProgramCounter);
                                                                auto EntryPoint = E.Evaluate(Operands[0]);
                                                                MainTable.Symbols[CurrentTable->Name].Value = EntryPoint;
                                                                break;
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
                                                            std::string Line = trim(OriginalLine);
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
                                                        auto MacroDefinition = CurrentTable->Macros.find(Mnemonic);
                                                        std::string MacroExpansion;
                                                        if(MacroDefinition != CurrentTable->Macros.end())
                                                            ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                        else if(CurrentTable != &MainTable)
                                                        {
                                                            MacroDefinition = MainTable.Macros.find(Mnemonic);
                                                            if(MacroDefinition != MainTable.Macros.end())
                                                                ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                        }
                                                        if(!MacroExpansion.empty())
                                                            Source.IncludeLiteral(Mnemonic, MacroExpansion);
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

                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        int x;
                                                        if((x = E.Evaluate(Operands[0])) < 0x10000)
                                                            ProgramCounter = x;
                                                        else
                                                            throw AssemblyException("Overflow: Address must be in range 0-FFFF", SEVERITY_Error);

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
                                                    case PROCESSOR:
                                                    {
                                                        Processor = OpCodeTable::CPUTable.at(Operands[0]);
                                                        break;
                                                    }
                                                    case ALIGN:
                                                    {
                                                        if(InAutoAlignedSub)
                                                            throw AssemblyException("ALIGN cannot be used inside an AUTO Aligned SUBROUTINE", SEVERITY_Error);
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("ALIGN Requires a single argument <alignment>", SEVERITY_Error);
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Align = E.Evaluate(Operands[0]);
                                                        if(Align != 2 && Align != 4 && Align != 8 && Align != 16 && Align != 32 && Align != 64 && Align != 129 && Align !=256)
                                                            throw AssemblyException("ALIGN must be 2,4,8,16,32,64,128 or 256", SEVERITY_Error);
                                                        ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
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
                                                    case SUB:
                                                    {
                                                        CurrentTable = &SubTables[Label];
                                                        for(int i = 0; i < Operands.size(); i++)
                                                        {
                                                            std::vector<std::string> SubOptions;
                                                            StringListToVector(Operands[i], SubOptions, '=');
                                                            auto Option = SubroutineOptionsLookup.find(SubOptions[0]);
                                                            if(Option == SubroutineOptionsLookup.end())
                                                                throw AssemblyException("Unrecognised SUBROUTINE option", SEVERITY_Warning);
                                                            switch(Option->second)
                                                            {
                                                                case SUBOPT_ALIGN:
                                                                {
                                                                    int Align;
                                                                    if(SubOptions[1] == "AUTO")
                                                                        Align = AlignFromSize(CurrentTable->CodeSize);
                                                                    else
                                                                    {
                                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                                        Align = E.Evaluate(SubOptions[1]);
                                                                    }
                                                                    ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                                                    CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                        ListingFile.Append();
                                                        break;
                                                    }
                                                    case ENDSUB:
                                                    {
                                                        CurrentTable = &MainTable;
                                                        ListingFile.Append();
                                                        break;
                                                    }
                                                    case MACRO:
                                                    {
                                                        ListingFile.Append();
                                                        while(Source.getLine(OriginalLine))
                                                        {
                                                            ListingFile.Append();
                                                            std::string Line = trim(OriginalLine);
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
                                                        auto MacroDefinition = CurrentTable->Macros.find(Mnemonic);
                                                        std::string MacroExpansion;
                                                        if(MacroDefinition != CurrentTable->Macros.end())
                                                            ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                        else if(CurrentTable != &MainTable)
                                                        {
                                                            MacroDefinition = MainTable.Macros.find(Mnemonic);
                                                            if(MacroDefinition != MainTable.Macros.end())
                                                                ExpandMacro(MacroDefinition->second, Operands, MacroExpansion);
                                                        }
                                                        ListingFile.Append();
                                                        if(!MacroExpansion.empty())
                                                            Source.IncludeLiteral(Mnemonic, MacroExpansion);
                                                        else
                                                            throw AssemblyException("Unknown OpCode", SEVERITY_Error);
                                                        break;
                                                    }
                                                    case ORG:
                                                    {
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        ProgramCounter = E.Evaluate(Operands[0]);

                                                        CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;

                                                        ListingFile.Append();
                                                        break;
                                                    }
                                                    case DB:
                                                    {
                                                        std::vector<std::uint8_t> Data;
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        for(auto& Operand : Operands)
                                                        {
                                                            if(Operand[0] == '\"')
                                                                StringToByteVector(Operand, Data);
                                                            else
                                                            {
                                                                int x = E.Evaluate(Operand);
                                                                if(x > 255)
                                                                    throw AssemblyException("Operand out of range (0-FF)", SEVERITY_Error);
                                                                Data.push_back(x & 0xFF);
                                                            }
                                                        }
                                                        CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                        ListingFile.Append(ProgramCounter, Data);
                                                        ProgramCounter += Data.size();
                                                        break;
                                                    }
                                                    case DW:
                                                    {
                                                        std::vector<std::uint8_t> Data;
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        for(auto& Operand : Operands)
                                                        {
                                                            int x = E.Evaluate(Operand);
                                                            Data.push_back((x >> 8) & 0xFF);
                                                            Data.push_back(x & 0xFF);
                                                        }
                                                        CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                        ListingFile.Append(ProgramCounter, Data);
                                                        ProgramCounter += Data.size();
                                                        break;
                                                    }
                                                    case PROCESSOR:
                                                    {
                                                        Processor = OpCodeTable::CPUTable.at(Operands[0]);
                                                        ListingFile.Append();
                                                        break;
                                                    }
                                                    case ALIGN:
                                                    {
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Align = E.Evaluate(Operands[0]);
                                                        ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                                        CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                        ListingFile.Append();
                                                        break;
                                                    }
                                                    case ASSERT:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("ASSERT Requires a single argument <expression>", SEVERITY_Error);
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Result = E.Evaluate(Operands[0]);
                                                        if (Result == 0)
                                                            throw AssemblyException("ASSERT Failed", SEVERITY_Error);
                                                        ListingFile.Append();
                                                        break;
                                                    }
                                                    case END:
                                                    {
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        EntryPoint = E.Evaluate(Operands[0]);
                                                        ListingFile.Append();
                                                        while(Source.getLine(OriginalLine))
                                                            ListingFile.Append();
                                                        break;
                                                    }
                                                    case LIST:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("LIST Requires a single argument <expression>", SEVERITY_Error);
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Result = E.Evaluate(Operands[0]);

                                                        if(Result == 0)
                                                            ListingFile.Enabled = false;
                                                        else
                                                            ListingFile.Enabled = true;
                                                        break;
                                                    }
                                                    case SYMBOLS:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("LIST Requires a single argument <expression>", SEVERITY_Error);
                                                        ExpressionEvaluator E(MainTable, ProgramCounter);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        int Result = E.Evaluate(Operands[0]);

                                                        if(Result == 0)
                                                            DumpSymbols = false;
                                                        else
                                                            DumpSymbols = true;
                                                        break;
                                                    }
                                                    default:
                                                        break;
                                                }
                                            }
                                            else
                                            {
                                                std::vector<std::uint8_t> Data;
                                                ExpressionEvaluator E(MainTable, ProgramCounter);
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
                                                            throw AssemblyException("Expected single operand of type Register", SEVERITY_Error);

                                                        uint16_t Register = E.Evaluate(Operands[0]);
                                                        if(Register > 15)
                                                            throw AssemblyException("Register out of range (0-F)", SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode | Register);
                                                        break;
                                                    }
                                                    case IMMEDIATE:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Byte", SEVERITY_Error);
                                                        uint16_t Byte = E.Evaluate(Operands[0]);
                                                        if(Byte > 0xFF && Byte < 0xFF80)
                                                            throw AssemblyException("Operand out of range (0-FF)", SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Byte & 0xFF);
                                                        break;
                                                    }
                                                    case SHORT_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Short Branch expected single operand", SEVERITY_Error);
                                                        uint16_t Address = E.Evaluate(Operands[0]);
                                                        if(((ProgramCounter + 1) & 0xFF00) != (Address & 0xFF00))
                                                            throw AssemblyException("Short Branch out of range", SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case LONG_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Long Branch expected single operand", SEVERITY_Error);
                                                        uint16_t Address = E.Evaluate(Operands[0]);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Address >> 8);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case INPUT_OUTPUT:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Port", SEVERITY_Error);

                                                        uint16_t Port = E.Evaluate(Operands[0]);
                                                        if(Port == 0 || Port > 7)
                                                            throw AssemblyException("Port out of range (1-7)", SEVERITY_Error);
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
                                                            throw AssemblyException("Expected single operand of type Register", SEVERITY_Error);

                                                        uint16_t Register = E.Evaluate(Operands[0]);
                                                        if(Register > 15)
                                                            throw AssemblyException("Register out of range (0-F)", SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF | Register);
                                                        break;
                                                    }
                                                    case EXTENDED_IMMEDIATE:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Byte", SEVERITY_Error);
                                                        uint16_t Byte = E.Evaluate(Operands[0]);
                                                        if(Byte > 0xFF && Byte < 0xFF80)
                                                            throw AssemblyException("Immediate operand out of range (0-FF)", SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        Data.push_back(Byte & 0xFF);
                                                        break;
                                                    }
                                                    case EXTENDED_SHORT_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Short Branch expected single operand", SEVERITY_Error);
                                                        uint16_t Address = E.Evaluate(Operands[0]);
                                                        if(((ProgramCounter + 2) & 0xFF00) != (Address & 0xFF00))
                                                            throw AssemblyException("Short Branch out of range", SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case EXTENDED_REGISTER_IMMEDIATE16:
                                                    {
                                                        if(Operands.size() != 2)
                                                            throw AssemblyException("Expected Register and Immediate operands", SEVERITY_Error);
                                                        uint16_t Register = E.Evaluate(Operands[0]);
                                                        uint16_t Word = E.Evaluate(Operands[1]);
                                                        if(Register > 15)
                                                            throw AssemblyException("Register out of range (0-F)", SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF | Register);
                                                        Data.push_back(Word >> 8);
                                                        Data.push_back(Word & 0xFF);
                                                        break;
                                                    }
                                                    default:
                                                        break;
                                                }

                                                CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());

                                                ListingFile.Append(ProgramCounter, Data);
                                                ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                            }
                                        }
                                        else
                                            ListingFile.Append();
                                    }
                                }
                            }
                            catch (AssemblyException Ex)
                            {
                                if(!Errors.Contains(Source.getName(), Source.getLineNumber(), Ex.Message, Ex.Severity))
                                {
                                    PrintError(Source.getName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                                    Errors.Push(Source.getName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                                }
                                if(Ex.SkipToOpCode.has_value())
                                {
                                    while(Source.getLine(OriginalLine))
                                    {
                                        std::string Line = trim(OriginalLine);
                                        ExpandDefines(Line, Defines);
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
                                    ListingFile.Append();
                                }
                            }

                        }
                    }
                    else // Empty line
                        if (Pass == 3)
                            ListingFile.Append();
                }
                catch (AssemblyException Ex)
                {
                    if(!Errors.Contains(Source.getName(), Source.getLineNumber(), Ex.Message, Ex.Severity))
                    {
                        PrintError(Source.getName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                        Errors.Push(Source.getName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                    }
                    if(Ex.SkipToOpCode.has_value())
                        throw; // AssemblyException is only thrown with a SkipToOpcode in an enclosed try / catch so this should never happen
                }
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
                    if(!EntryPoint.has_value())
                        throw AssemblyException("END Statement is missing", SEVERITY_Warning);

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
            PrintError(Ex.Message, Ex.Severity);
            Errors.Push(Ex.Message, Ex.Severity);
            if(Ex.SkipToOpCode.has_value())
                throw; // AssemblyException is only thrown with a SkipToOpcode in an enclosed try / catch so this should never happen
        }
    } // for(int Pass = 1; Pass <= 3 && Errors.count(SEVERITY_Error) == 0; Pass++)...

    ListingFile.AppendGlobalErrors();

    if(DumpSymbols)
    {
        fmt::print("\n");
#if DEBUG
        PrintSymbols("Global Symbols", MainTable);
#endif
        ListingFile.AppendSymbols("Global Symbols", MainTable);
        for(auto& Table : SubTables)
        {
#if DEBUG
            PrintSymbols(Table.first, Table.second);
#endif
            ListingFile.AppendSymbols(Table.first, Table.second);
        }
    }

    int TotalWarnings = Errors.count(SEVERITY_Warning);
    int TotalErrors = Errors.count(SEVERITY_Error);

    fmt::print("\n");
    fmt::print("{count:4} Warnings\n",     fmt::arg("count", TotalWarnings));
    fmt::print("{count:4} Errors\n",       fmt::arg("count", TotalErrors));
    fmt::print("\n");

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

#if DEBUG
    DumpCode(Code);
#endif

    return TotalErrors == 0 && TotalWarnings == 0;
}

#if DEBUG
void DumpCode(const std::map<uint16_t, std::vector<uint8_t>>& Code)
{
    fmt::print("Code Dump\n");
    for(auto &Segment : Code)
    {
        if(Segment.second.size() > 0)
        {
            uint16_t StartAddress = Segment.first & 0xFFF0;
            int skip = Segment.first & 0x000F;
            for(int i = 0; i < Segment.second.size() + skip; i++)
            {
                if(i % 16 == 0)
                    fmt::print("\n{Addr:04X}  ", fmt::arg("Addr", StartAddress + i));
                if(i < skip)
                    fmt::print("   ");
                else
                    fmt::print("{Data:02X} ", fmt::arg("Data", Segment.second.at(i - skip)));
            }
        }
        fmt::print("\n");
    }
}
#endif

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
void PrintError(const std::string& SourceFileName, const int LineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity)
{
    try // Source may not contain anything...
    {
        std::string FileName = fs::path(SourceFileName);
        if(FileName.length() > 20)
            FileName = FileName.substr(0, 17) + "...";

        fmt::print("[{filename:22}({linenumber:5})] {line}\n",
                   fmt::arg("filename", FileName),
                   fmt::arg("linenumber", LineNumber),
                   fmt::arg("line", Line)
                  );
        fmt::print("***************{severity:*>15}: {message}\n",
                   fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                   fmt::arg("message", Message));
    }
    catch(...)
    {
        fmt::print("***************{severity:*>15}: {message}\n",
                   fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                   fmt::arg("message", Message));
    }
}

void PrintError(const std::string& Message, AssemblyErrorSeverity Severity)
{
    fmt::print("***************{severity:*>15}: {message}\n",
               fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
               fmt::arg("message", Message));
}

//!
//! \brief PrintSymbols
//! \param Name
//! \param Symbols
//!
//! Print the given Symbol Table
//!
void PrintSymbols(const std::string& Name, const SymbolTable& Blob)
{
    fmt::print("{Name:-^116}\n", fmt::arg("Name", Name));

    int c = 0;
    for(auto& Symbol : Blob.Symbols)
        if(!Symbol.second.HideFromSymbolTable)
        {
            fmt::print("{Name:15} ", fmt::arg("Name", Symbol.first));
            if(Symbol.second.Value.has_value())
                fmt::print("{Address:04X}", fmt::arg("Address", Symbol.second.Value.value()));
            if(++c % 5 == 0)
                fmt::print("\n");
            else
                fmt::print("    ");
        }
    fmt::print("\n");
}
