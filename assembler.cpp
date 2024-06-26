#include "assembler.h"
#include "symboltable.h"
#include "assemblyexpressionevaluator.h"
#include "binarywriter_idiot4.h"
#include "binarywriter_intelhex.h"
#include "binarywriter_elfos.h"
#include "binarywriter_binary.h"
#include "expressionexception.h"
#include "listingfilewriter.h"
#include "sourcecodereader.h"
#include "symboltable.h"
#include "utils.h"

const std::map<std::string, Assembler::PreProcessorControlEnum> Assembler::PreProcessorControlLookup =
{
    { "line",      Assembler::PreProcessorControlEnum::PP_LINE      },
    { "processor", Assembler::PreProcessorControlEnum::PP_PROCESSOR },
    { "list",      Assembler::PreProcessorControlEnum::PP_LIST      },
    { "symbols",   Assembler::PreProcessorControlEnum::PP_SYMBOLS   }
};

const std::map<std::string, Assembler::SubroutineOptionsEnum> Assembler::SubroutineOptionsLookup =
{
    { "ALIGN",  Assembler::SubroutineOptionsEnum::SUBOPT_ALIGN  },
    { "STATIC", Assembler::SubroutineOptionsEnum::SUBOPT_STATIC },
    { "PAD",    Assembler::SubroutineOptionsEnum::SUBOPT_PAD    }
};

const std::map<std::string, Assembler::OutputFormatEnum> Assembler::OutputFormatLookup =
{
    { "INTEL_HEX", Assembler::OutputFormatEnum::INTEL_HEX },
    { "IDIOT4",    Assembler::OutputFormatEnum::IDIOT4    },
    { "ELFOS",     Assembler::OutputFormatEnum::ELFOS     },
    { "BIN",       Assembler::OutputFormatEnum::BIN       }
};

Assembler::Assembler(const std::string& FileName, CPUTypeEnum& InitialProcessor, bool ListingEnabled, bool DumpSymbols, bool& NoRegisters, bool& NoPorts, const std::vector<OutputFormatEnum>& BinMode) :
    FileName(FileName),
    InitialProcessor(InitialProcessor),
    NoRegisters(NoRegisters),
    NoPorts(NoPorts),
    BinMode(BinMode)
{
    this->ListingEnabled = ListingEnabled;
    this->DumpSymbols = DumpSymbols;
}

//!
//! \brief assemble
//! \param FileName
//! \return
//!
//! Main Assembler
//!
bool Assembler::Run()
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
    int TotalPadBytes = 0;
    int TotelOptimisedBytes = 0;

    for(int Pass = 1; Pass <= 3 && Errors.count(AssemblyErrorSeverity::SEVERITY_Error) == 0; Pass++)
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
        TotalPadBytes = 0;

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
                            case PreProcessorControlEnum::PP_LINE:
                            {
                                std::smatch MatchResult;
                                std::string NewFile;
                                if(regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-")))
                                {
                                    CurrentFile = MatchResult[1];
                                    LineNumber = stoi(MatchResult[2]);
                                }
                                else
                                    throw AssemblyException("Bad line directive received from Pre-Processor", AssemblyErrorSeverity::SEVERITY_Error);
                                break;
                            }
                            case PreProcessorControlEnum::PP_PROCESSOR:
                            {
                                if(Pass == 3)
                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                ToUpper(Expression);
                                if(OpCodeTable::CPUTable.find(Expression) != OpCodeTable::CPUTable.end())
                                    Processor = OpCodeTable::CPUTable.at(Expression);
                                else
                                    throw AssemblyException("Bad processor directive received from Pre-Processor", AssemblyErrorSeverity::SEVERITY_Error);
                                if(!Source.InMacro())
                                    LineNumber++;
                                break;
                            }
                            case PreProcessorControlEnum::PP_LIST:
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
                                        throw AssemblyException("Bad list directive received from Pre-Processor", AssemblyErrorSeverity::SEVERITY_Error);
                                }
                                if(!Source.InMacro())
                                    LineNumber++;
                                break;
                            case PreProcessorControlEnum::PP_SYMBOLS:
                                if(Pass == 3)
                                {
                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                    ToUpper(Expression);
                                    if(Expression == "ON")
                                        DumpSymbols = true;
                                    else if(Expression == "OFF")
                                        DumpSymbols = false;
                                    else
                                        throw AssemblyException("Bad symbols directive received from Pre-Processor", AssemblyErrorSeverity::SEVERITY_Error);
                                }
                                if(!Source.InMacro())
                                    LineNumber++;
                                break;
                        }
                        continue; // Go back to start of getLine loop - control statements have no further processing.
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
                                throw AssemblyException("Subroutine definition must be within a single source file", AssemblyErrorSeverity::SEVERITY_Error);
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
                                        if(OpCode.value().OpCodeType == OpCodeTypeEnum::PSEUDO_OP)
                                        {
                                            switch(OpCode.value().OpCode)
                                            {
                                                case OpCodeEnum::SUB:
                                                    if(InSub)
                                                        throw AssemblyException("SUBROUTINEs cannot be nested", AssemblyErrorSeverity::SEVERITY_Error, OpCodeEnum::ENDSUB);

                                                    if(Label.empty())
                                                        throw AssemblyException("SUBROUTINE requires a Label", AssemblyErrorSeverity::SEVERITY_Error, OpCodeEnum::ENDSUB);

                                                    if(SubTables.find(Label) != SubTables.end())
                                                        throw AssemblyException(fmt::format("Subroutine '{Label}' is already defined", fmt::arg("Label", Label)), AssemblyErrorSeverity::SEVERITY_Error, OpCodeEnum::ENDSUB);

                                                    InSub = true;
                                                    SubDefinitionFile = CurrentFile;
                                                    SubTables.insert(std::pair<std::string, SymbolTable>(Label, SymbolTable()));
                                                    CurrentTable = &SubTables[Label];
                                                    SubroutineSize = 0;
                                                    break;
                                                case OpCodeEnum::ENDSUB:
                                                {
                                                    if(!InSub)
                                                        throw AssemblyException("ENDSUB without matching SUB", AssemblyErrorSeverity::SEVERITY_Error);
                                                    InSub = false;

                                                    CurrentTable->CodeSize = SubroutineSize;
                                                    CurrentTable = &MainTable;
                                                    break;
                                                }
                                                case OpCodeEnum::MACRO:
                                                {
                                                    if(CurrentTable->Macros.find(Label) != CurrentTable->Macros.end())
                                                        throw AssemblyException(fmt::format("Macro '{Macro}' is already defined", fmt::arg("Macro", Label)), AssemblyErrorSeverity::SEVERITY_Error, OpCodeEnum::ENDMACRO);
                                                    if(OpCodeTable::OpCode.find(Label) != OpCodeTable::OpCode.end())
                                                        throw AssemblyException(fmt::format("Cannot use reserved word '{OpCode}' as a Macro name", fmt::arg("OpCode", Label)), AssemblyErrorSeverity::SEVERITY_Error, OpCodeEnum::ENDMACRO);
                                                    Macro& MacroDefinition = CurrentTable->Macros[Label];
                                                    std::regex ArgMatch(R"(^[A-Z_][A-Z0-9_]*$)");
                                                    for(auto& Arg : Operands)
                                                    {
                                                        std::string Argument(Arg);
                                                        ToUpper(Argument);

                                                        if(OpCodeTable::OpCode.find(Argument) != OpCodeTable::OpCode.cend())
                                                            throw AssemblyException(fmt::format("Cannot use reserved word '{OpCode}' as a Macro parameter", fmt::arg("OpCode", Argument)), AssemblyErrorSeverity::SEVERITY_Error, OpCodeEnum::ENDMACRO);

                                                        if(std::regex_match(Argument, ArgMatch))
                                                            if(std::find(MacroDefinition.Arguments.begin(), MacroDefinition.Arguments.end(), Argument) == MacroDefinition.Arguments.end())
                                                                MacroDefinition.Arguments.push_back(Argument);
                                                            else
                                                                throw AssemblyException("Macro arguments must be unique", AssemblyErrorSeverity::SEVERITY_Error);
                                                        else
                                                            throw AssemblyException(fmt::format("Invalid argument name: '{Name}'", fmt::arg("Name", Argument)), AssemblyErrorSeverity::SEVERITY_Error);
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
                                                                    && PreProcessorControlLookup.at(ControlWord) == PreProcessorControlEnum::PP_LINE
                                                                    && regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-"))
                                                                    && CurrentFile != MatchResult[1])
                                                                throw AssemblyException("Macro definition must be within a single source file", AssemblyErrorSeverity::SEVERITY_Error);
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
                                                            throw AssemblyException("Cannot define a label inside a macro", AssemblyErrorSeverity::SEVERITY_Error);
                                                        if(OpCode.has_value() && OpCode.value().OpCode == OpCodeEnum::ENDMACRO)
                                                            break;
                                                        fmt::println(Expansion, OriginalLine);
                                                    }

                                                    MacroDefinition.Expansion = Expansion.str();
                                                    break;
                                                }
                                                case OpCodeEnum::ENDMACRO:
                                                    throw AssemblyException("ENDMACRO without opening MACRO pseudo-op", AssemblyErrorSeverity::SEVERITY_Error);
                                                    break;
                                                case OpCodeEnum::MACROEXPANSION:
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
                                                    if(!MacroExpansion.empty())
                                                        Source.InsertMacro(Mnemonic, MacroExpansion);
                                                    else
                                                        throw AssemblyException("Unknown OpCode", AssemblyErrorSeverity::SEVERITY_Error);
                                                    break;
                                                }
                                                case OpCodeEnum::DB:
                                                {
                                                    for(auto& Operand : Operands)
                                                        switch(Operand[0])
                                                        {
                                                            case '\"':
                                                            {
                                                                std::vector<std::uint8_t> Data;
                                                                StringToByteVector(Operand, Data);
                                                                SubroutineSize += Data.size();
                                                                break;
                                                            }
                                                            case '@':
                                                            {
                                                                std::string FileName = GetFileName(&Operand[1]);
                                                                if(!fs::exists(FileName))
                                                                    throw AssemblyException(fmt::format("File Not Found: '{Filename}'", fmt::arg("FileName", FileName)), AssemblyErrorSeverity::SEVERITY_Error);
                                                                SubroutineSize += fs::file_size(FileName);
                                                                break;
                                                            }
                                                            default:
                                                            {
                                                                SubroutineSize++;
                                                                break;
                                                            }
                                                        }
                                                    break;
                                                }
                                                case OpCodeEnum::DW:
                                                {
                                                    SubroutineSize += Operands.size() * 2;
                                                    break;
                                                }
                                                case OpCodeEnum::DL:
                                                {
                                                    SubroutineSize += Operands.size() * 4;
                                                    break;
                                                }
                                                case OpCodeEnum::DQ:
                                                {
                                                    if(sizeof(long) < 8)
                                                        throw AssemblyException("DQ Not supported in this build", AssemblyErrorSeverity::SEVERITY_Error);
                                                    SubroutineSize += Operands.size() * 8;
                                                    break;
                                                }
                                                case OpCodeEnum::RB:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() > 1)
                                                        throw AssemblyException("RB takes one optional argument {count}", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    SubroutineSize += Count;
                                                    break;
                                                }
                                                case OpCodeEnum::RW:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() > 1)
                                                        throw AssemblyException("RW takes one optional argument {count}", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    SubroutineSize += Count * 2;
                                                    break;
                                                }
                                                case OpCodeEnum::RL:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() > 1)
                                                        throw AssemblyException("RL takes one optional argument {count}", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    SubroutineSize += Count * 4;
                                                    break;
                                                }
                                                case OpCodeEnum::RQ:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() > 1)
                                                        throw AssemblyException("RQ takes one optional argument {count}", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    SubroutineSize += Count * 8;
                                                    break;
                                                }
                                                case OpCodeEnum::END:
                                                    if(InSub)
                                                        throw AssemblyException("END cannot appear inside a SUBROUTINE", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("END requires a single argument <entry point>", AssemblyErrorSeverity::SEVERITY_Error);
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
                                                throw AssemblyException("Instruction not supported on selected processor", AssemblyErrorSeverity::SEVERITY_Error);
                                            SubroutineSize += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                        }
                                    }
                                    break;
                                }
                                case 2: // Generate Symbol Tables
                                {
                                    if(!Label.empty() && UnReferencedSubs.count(Label)==0 && (!OpCode.has_value() || OpCode.value().OpCode != OpCodeEnum::MACRO))
                                    {
                                        if(CurrentTable->Symbols.find(Label) == CurrentTable->Symbols.end())
                                            CurrentTable->Symbols[Label].Value = ProgramCounter;
                                        else
                                        {
                                            auto& Symbol = CurrentTable->Symbols[Label];
                                            if(Symbol.Value.has_value())
                                                throw AssemblyException(fmt::format("Label '{Label}' is already defined", fmt::arg("Label", Label)), AssemblyErrorSeverity::SEVERITY_Error);
                                            Symbol.Value = ProgramCounter;
                                        }
                                    }

                                    if(OpCode)
                                    {
                                        if(OpCode.value().OpCodeType == OpCodeTypeEnum::PSEUDO_OP)
                                        {
                                            switch(OpCode.value().OpCode)
                                            {
                                                case OpCodeEnum::EQU:
                                                {
                                                    if(Label.empty())
                                                        throw AssemblyException("EQU requires a Label", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("EQU Requires a single argument <value>", AssemblyErrorSeverity::SEVERITY_Error);

                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        long Value = E.Evaluate(Operands[0]);
                                                        CurrentTable->Symbols[Label].Value = Value;
                                                    }
                                                    catch (ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                    }
                                                    break;
                                                }
                                                case OpCodeEnum::SUB:
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
                                                                            case PreProcessorControlEnum::PP_LINE:
                                                                            {
                                                                                std::smatch MatchResult;
                                                                                std::string NewFile;
                                                                                if(regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-")))
                                                                                {
                                                                                    CurrentFile = MatchResult[1];
                                                                                    LineNumber = stoi(MatchResult[2]);
                                                                                }
                                                                                else
                                                                                    throw AssemblyException("Bad line directive received from Pre-Processor", AssemblyErrorSeverity::SEVERITY_Error);
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
                                                                    if(OpCode.has_value() && OpCode.value().OpCode == OpCodeEnum::ENDSUB)
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
                                                                throw AssemblyException("Unrecognised SUBROUTINE option", AssemblyErrorSeverity::SEVERITY_Warning);
                                                            switch(Option->second)
                                                            {
                                                                case SubroutineOptionsEnum::SUBOPT_ALIGN:
                                                                    if(SubOptions.size() != 2)
                                                                        throw AssemblyException("Unrecognised SUBROUTINE ALIGN option", AssemblyErrorSeverity::SEVERITY_Error);
                                                                    else
                                                                    {
                                                                        long Align;
                                                                        if(SubOptions[1] == "AUTO")
                                                                        {
                                                                            if(((ProgramCounter + CurrentTable->CodeSize) & 0xFF00) == (ProgramCounter & 0xFF00))
                                                                                Align = 0;
                                                                            else
                                                                                Align = AlignFromSize(CurrentTable->CodeSize);
                                                                            InAutoAlignedSub = true;
                                                                        }
                                                                        else if(!SetAlignFromKeyword(SubOptions[1], Align))
                                                                        {
                                                                            try
                                                                            {
                                                                                AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                                                Align = E.Evaluate(SubOptions[1]);
                                                                                if(Align != 2 && Align != 4 && Align != 8 && Align != 16 && Align != 32 && Align != 64 && Align != 128 && Align !=256)
                                                                                    throw AssemblyException("SUBROUTINE ALIGN must be 2,4,8,16,32,64,128,256 or AUTO", AssemblyErrorSeverity::SEVERITY_Error);
                                                                            }
                                                                            catch (ExpressionException Ex)
                                                                            {
                                                                                throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                                            }
                                                                        }
                                                                        ProgramCounter = ProgramCounter + GetAlignExtraBytes(ProgramCounter, Align);
                                                                        MainTable.Symbols[Label].Value = ProgramCounter;
                                                                    }
                                                                    break;

                                                                case SubroutineOptionsEnum::SUBOPT_STATIC:
                                                                    if(SubOptions.size() != 1)
                                                                        throw AssemblyException("SUBROUTINE STATIC option does not take any arguments", AssemblyErrorSeverity::SEVERITY_Error);
                                                                    CurrentTable->Static = true;
                                                                    break;

                                                                case SubroutineOptionsEnum::SUBOPT_PAD:
                                                                    break;

                                                                default:
                                                                    throw AssemblyException("Unrecognised SUBROUTINE option", AssemblyErrorSeverity::SEVERITY_Warning);
                                                                    break;
                                                            }
                                                        }
                                                        CurrentTable->Symbols[Label].Value = ProgramCounter;
                                                        break;
                                                    }
                                                    break;
                                                }
                                                case OpCodeEnum::ENDSUB:
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
                                                                long EntryPoint = E.Evaluate(Operands[0]);
                                                                MainTable.Symbols[CurrentTable->Name].Value = EntryPoint;
                                                                //CurrentTable->Symbols[CurrentTable->Name].Value = EntryPoint;
                                                                break;
                                                            }
                                                            catch (ExpressionException Ex)
                                                            {
                                                                throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                            }

                                                        default:
                                                            throw AssemblyException("Incorrect number of arguments", AssemblyErrorSeverity::SEVERITY_Error);
                                                    }
                                                    CurrentTable = &MainTable;
                                                    break;
                                                }
                                                case OpCodeEnum::MACRO:
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
                                                        if(OpCode.has_value() && OpCode.value().OpCode == OpCodeEnum::ENDMACRO)
                                                            break;
                                                    }
                                                    break;
                                                }
                                                case OpCodeEnum::MACROEXPANSION:
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
                                                    if(!MacroExpansion.empty())
                                                        Source.InsertMacro(Mnemonic, MacroExpansion);
                                                    else
                                                        throw AssemblyException("Unknown OpCode", AssemblyErrorSeverity::SEVERITY_Error);
                                                    break;
                                                }
                                                case OpCodeEnum::ORG:
                                                {
                                                    if(CurrentTable != &MainTable)
                                                        throw AssemblyException("ORG Cannot be used in a SUBROUTINE", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if(Operands.size() != 1)
                                                        throw AssemblyException("ORG Requires a single argument <address>", AssemblyErrorSeverity::SEVERITY_Error);
                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        long x = E.Evaluate(Operands[0]);
                                                        if(x >= 0 && x < 0x10000)
                                                            ProgramCounter = x;
                                                        else
                                                            throw AssemblyException("Overflow: Address must be in range 0-FFFF", AssemblyErrorSeverity::SEVERITY_Error);
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                    }
                                                    if(!Label.empty())
                                                        CurrentTable->Symbols[Label].Value = ProgramCounter;

                                                    break;
                                                }
                                                case OpCodeEnum::DB:
                                                {
                                                    for(auto& Operand : Operands)
                                                        switch(Operand[0])
                                                        {
                                                            case '\"':
                                                            {
                                                                std::vector<std::uint8_t> Data;
                                                                StringToByteVector(Operand, Data);
                                                                ProgramCounter += Data.size();
                                                                break;
                                                            }
                                                            case '@':
                                                            {
                                                                std::string FileName = GetFileName(&Operand[1]);
                                                                ProgramCounter += fs::file_size(FileName);
                                                                break;
                                                            }
                                                            default:
                                                                ProgramCounter++;
                                                                break;
                                                        }
                                                    break;
                                                }
                                                case OpCodeEnum::DW:
                                                {
                                                    ProgramCounter += Operands.size() * 2;
                                                    break;
                                                }
                                                case OpCodeEnum::DL:
                                                {
                                                    ProgramCounter += Operands.size() * 4;
                                                    break;
                                                }
                                                case OpCodeEnum::DQ:
                                                {
                                                    ProgramCounter += Operands.size() * 8;
                                                    break;
                                                }
                                                case OpCodeEnum::RB:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ProgramCounter += Count;
                                                    break;
                                                }
                                                case OpCodeEnum::RW:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ProgramCounter += Count * 2;
                                                    break;
                                                }
                                                case OpCodeEnum::RL:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ProgramCounter += Count * 4;
                                                    break;
                                                }
                                                case OpCodeEnum::RQ:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ProgramCounter += Count * 8;
                                                    break;
                                                }
                                                case OpCodeEnum::ALIGN:
                                                {
                                                    if(InAutoAlignedSub)
                                                        throw AssemblyException("ALIGN cannot be used inside an AUTO Aligned SUBROUTINE", AssemblyErrorSeverity::SEVERITY_Error);
                                                    if(Operands.size() == 0 || Operands.size() > 2)
                                                        throw AssemblyException("ALIGN Requires a single argument <alignment>", AssemblyErrorSeverity::SEVERITY_Error);
                                                    try
                                                    {
                                                        long Align;
                                                        if(!SetAlignFromKeyword(Operands[0], Align))
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Align = E.Evaluate(Operands[0]);
                                                            if(Align != 2 && Align != 4 && Align != 8 && Align != 16 && Align != 32 && Align != 64 && Align != 128 && Align !=256)
                                                                throw AssemblyException("ALIGN must be 2,4,8,16,32,64,128 or 256", AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                        ProgramCounter = ProgramCounter + GetAlignExtraBytes(ProgramCounter, Align);
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                    }

                                                    if(!Label.empty())
                                                        CurrentTable->Symbols[Label].Value = ProgramCounter;
                                                    break;
                                                }
                                                case OpCodeEnum::END:
                                                    while(Source.getLine(OriginalLine))
                                                        ;
                                                default:
                                                    break;
                                            }
                                        }
                                        else if(OpCode && OpCode.value().OpCodeType != OpCodeTypeEnum::PSEUDO_OP)
                                            ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                    }
                                    break;
                                }
                                case 3: // Generate Code
                                {
                                    if(OpCode)
                                    {
                                        if(OpCode.value().OpCodeType == OpCodeTypeEnum::PSEUDO_OP)
                                        {
                                            switch(OpCode.value().OpCode)
                                            {
                                                case OpCodeEnum::EQU:
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    break;
                                                case OpCodeEnum::SUB:
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
                                                                            case PreProcessorControlEnum::PP_LINE:
                                                                            {
                                                                                std::smatch MatchResult;
                                                                                std::string NewFile;
                                                                                if(regex_match(Expression, MatchResult, std::regex(R"-(^"(.*)" ([0-9]+)$)-")))
                                                                                {
                                                                                    CurrentFile = MatchResult[1];
                                                                                    LineNumber = stoi(MatchResult[2]) - 1;
                                                                                }
                                                                                else
                                                                                    throw AssemblyException("Bad line directive received from Pre-Processor", AssemblyErrorSeverity::SEVERITY_Error);
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
                                                                    if(OpCode.has_value() && OpCode.value().OpCode == OpCodeEnum::ENDSUB)
                                                                        break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    else // Assemble subroutine
                                                    {
                                                        CurrentTable = &SubTables[Label];
                                                        SubDefinitionFile = CurrentFile;
                                                        long Align = 0;
                                                        bool Pad = false;
                                                        int PadByte = 0;
                                                        for(int i = 0; i < Operands.size(); i++)
                                                        {
                                                            std::vector<std::string> SubOptions;
                                                            StringListToVector(Operands[i], SubOptions, '=');
                                                            ToUpper(SubOptions[0]);
                                                            auto Option = SubroutineOptionsLookup.find(SubOptions[0]);
                                                            if(Option == SubroutineOptionsLookup.end())
                                                                throw AssemblyException("Unrecognised SUBROUTINE option", AssemblyErrorSeverity::SEVERITY_Warning);
                                                            try
                                                            {
                                                                switch(Option->second)
                                                                {
                                                                    case SubroutineOptionsEnum::SUBOPT_ALIGN:
                                                                        if(SubOptions[1] == "AUTO")
                                                                        {
                                                                            if(((ProgramCounter + CurrentTable->CodeSize) & 0xFF00) == (ProgramCounter & 0xFF00))
                                                                                Align = 0;
                                                                            else
                                                                                Align = AlignFromSize(CurrentTable->CodeSize);
                                                                        }
                                                                        else if(!SetAlignFromKeyword(SubOptions[1], Align))
                                                                        {
                                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                                            Align = E.Evaluate(SubOptions[1]);
                                                                        }

                                                                        break;
                                                                    case SubroutineOptionsEnum::SUBOPT_STATIC:
                                                                        break;
                                                                    case SubroutineOptionsEnum::SUBOPT_PAD:
                                                                    {
                                                                        switch(SubOptions.size())
                                                                        {
                                                                            case 1:
                                                                                PadByte = 0;
                                                                                Pad = true;
                                                                                break;
                                                                            case 2:
                                                                            {
                                                                                AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                                                if(CurrentTable != &MainTable)
                                                                                    E.AddLocalSymbols(CurrentTable);
                                                                                PadByte = E.Evaluate(SubOptions[1]);
                                                                                Pad = true;
                                                                                break;
                                                                            }
                                                                            default:
                                                                                throw AssemblyException("Incorrect number of arguments to PAD option", AssemblyErrorSeverity::SEVERITY_Error);
                                                                                break;
                                                                        }
                                                                        break;
                                                                    }
                                                                }
                                                            }
                                                            catch(ExpressionException Ex)
                                                            {
                                                                throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                            }
                                                        }
                                                        if(Align > 0)
                                                        {
                                                            int BytesToAdd = GetAlignExtraBytes(ProgramCounter, Align);
                                                            if(Pad)
                                                                for(int i = 0; i < BytesToAdd; i++)
                                                                    CurrentCode->second.push_back(PadByte);
                                                            else
                                                                CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter + GetAlignExtraBytes(ProgramCounter, Align), {})).first;
                                                            ProgramCounter = ProgramCounter + BytesToAdd;
                                                            TotalPadBytes += BytesToAdd;
                                                        }
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    }
                                                    break;
                                                }
                                                case OpCodeEnum::ENDSUB:
                                                {
                                                    CurrentTable = &MainTable;
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    break;
                                                }
                                                case OpCodeEnum::MACRO:
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
                                                        if(OpCode.has_value() && OpCode.value().OpCode == OpCodeEnum::ENDMACRO)
                                                            break;
                                                    }
                                                    break;
                                                }
                                                case OpCodeEnum::MACROEXPANSION:
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
                                                    if(!MacroExpansion.empty())
                                                        Source.InsertMacro(Mnemonic, MacroExpansion);
                                                    else
                                                        throw AssemblyException("Unknown OpCode", AssemblyErrorSeverity::SEVERITY_Error);
                                                    break;
                                                }
                                                case OpCodeEnum::ORG:
                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        ProgramCounter = E.Evaluate(Operands[0]);
                                                        CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                    }
                                                    break;
                                                case OpCodeEnum::DB:
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                    if(CurrentTable != &MainTable)
                                                        E.AddLocalSymbols(CurrentTable);
                                                    for(auto& Operand : Operands)
                                                        switch(Operand[0])
                                                        {
                                                            case  '\"':
                                                            {
                                                                StringToByteVector(Operand, Data);
                                                                break;
                                                            }
                                                            case '@':
                                                            {
                                                                std::string FileName = GetFileName(&Operand[1]);
                                                                std::ifstream Input(FileName, std::ifstream::binary);
                                                                std::copy(std::istreambuf_iterator<char>(Input), std::istreambuf_iterator<char>(), std::back_inserter(Data));
                                                                break;
                                                            }
                                                            default:
                                                                try
                                                                {
                                                                    long x = E.Evaluate(Operand);
                                                                    if(x > 255)
                                                                        throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FF, got: ${value:X})", fmt::arg("value", x)), AssemblyErrorSeverity::SEVERITY_Error);
                                                                    Data.push_back(x & 0xFF);
                                                                }
                                                                catch(ExpressionException Ex)
                                                                {
                                                                    throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                                }
                                                                break;
                                                        }
                                                    CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                    ProgramCounter += Data.size();
                                                    break;
                                                }
                                                case OpCodeEnum::DW:
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                    if(CurrentTable != &MainTable)
                                                        E.AddLocalSymbols(CurrentTable);
                                                    for(auto& Operand : Operands)
                                                        try
                                                        {
                                                            long x = E.Evaluate(Operand);
                                                            Data.push_back((x >> 8) & 0xFF);
                                                            Data.push_back(x & 0xFF);
                                                        }
                                                        catch(ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                    ProgramCounter += Data.size();
                                                    break;
                                                }
                                                case OpCodeEnum::DL:
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                    if(CurrentTable != &MainTable)
                                                        E.AddLocalSymbols(CurrentTable);
                                                    for(auto& Operand : Operands)
                                                        try
                                                        {
                                                            long x = E.Evaluate(Operand);
                                                            Data.push_back((x >> 24) & 0xFF);
                                                            Data.push_back((x >> 16) & 0xFF);
                                                            Data.push_back((x >> 8) & 0xFF);
                                                            Data.push_back(x & 0xFF);
                                                        }
                                                        catch(ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                    ProgramCounter += Data.size();
                                                    break;
                                                }
                                                case OpCodeEnum::DQ:
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                    if(CurrentTable != &MainTable)
                                                        E.AddLocalSymbols(CurrentTable);
                                                    for(auto& Operand : Operands)
                                                        try
                                                        {
                                                            long x = E.Evaluate(Operand);
                                                            Data.push_back((x >> 56) & 0xFF);
                                                            Data.push_back((x >> 48) & 0xFF);
                                                            Data.push_back((x >> 40) & 0xFF);
                                                            Data.push_back((x >> 32) & 0xFF);
                                                            Data.push_back((x >> 24) & 0xFF);
                                                            Data.push_back((x >> 16) & 0xFF);
                                                            Data.push_back((x >>  8) & 0xFF);
                                                            Data.push_back(x & 0xFF);
                                                        }
                                                        catch(ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    CurrentCode->second.insert(CurrentCode->second.end(), Data.begin(), Data.end());
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, Data);
                                                    ProgramCounter += Data.size();
                                                    break;
                                                }
                                                case OpCodeEnum::RB:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, {});
                                                    ProgramCounter += Count;
                                                    CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                    break;
                                                }
                                                case OpCodeEnum::RW:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, {});
                                                    ProgramCounter += Count * 2;
                                                    CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                    break;
                                                }
                                                case OpCodeEnum::RL:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, {});
                                                    ProgramCounter += Count * 4;
                                                    CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                    break;
                                                }
                                                case OpCodeEnum::RQ:
                                                {
                                                    long Count = 1;
                                                    if (Operands.size() == 1)
                                                    {
                                                        try
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Count = E.Evaluate((Operands[0]));
                                                        }
                                                        catch (ExpressionException Ex)
                                                        {
                                                            throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                    }
                                                    ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro(), ProgramCounter, {});
                                                    ProgramCounter += Count * 8;
                                                    CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                    break;
                                                }
                                                case OpCodeEnum::ALIGN:
                                                    try
                                                    {
                                                        long Align;
                                                        if(!SetAlignFromKeyword(Operands[0], Align))
                                                        {
                                                            AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                            if(CurrentTable != &MainTable)
                                                                E.AddLocalSymbols(CurrentTable);
                                                            Align = E.Evaluate(Operands[0]);
                                                        }

                                                        bool Pad = false;
                                                        int PadByte = 0;
                                                        if(Operands.size() == 2)
                                                        {
                                                            std::vector<std::string> SubOptions;
                                                            StringListToVector(Operands[1], SubOptions, '=');
                                                            ToUpper(SubOptions[0]);
                                                            if(SubOptions[0] == "PAD")
                                                            {
                                                                AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                                if(CurrentTable != &MainTable)
                                                                    E.AddLocalSymbols(CurrentTable);
                                                                PadByte = E.Evaluate(SubOptions[1]);
                                                                Pad = true;
                                                            }
                                                        }
                                                        int ExtraBytes = GetAlignExtraBytes(Align, ProgramCounter);
                                                        if(Pad)
                                                            for(int i = 0; i < GetAlignExtraBytes(ProgramCounter, Align); i++)
                                                                CurrentCode->second.push_back(PadByte);
                                                        else
                                                            CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                                        ProgramCounter = ProgramCounter + GetAlignExtraBytes(ProgramCounter, Align);
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                        break;
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                    }
                                                case OpCodeEnum::ASSERT:
                                                {
                                                    if(Operands.size() ==0 || Operands.size() > 2)
                                                        throw AssemblyException("ASSERT Requires a single argument <expression>, and optionam <message>", AssemblyErrorSeverity::SEVERITY_Error);
                                                    try
                                                    {
                                                        AssemblyExpressionEvaluator E(MainTable, ProgramCounter, Processor);
                                                        if(CurrentTable != &MainTable)
                                                            E.AddLocalSymbols(CurrentTable);
                                                        long Result = E.Evaluate(Operands[0]);
                                                        if (Result == 0)
                                                        {
                                                            if(Operands.size() == 2)
                                                                throw AssemblyException(fmt::format("ASSERT Failed: {Message}", fmt::arg("Message", Operands[1])), AssemblyErrorSeverity::SEVERITY_Error);
                                                            else
                                                                throw AssemblyException("ASSERT Failed", AssemblyErrorSeverity::SEVERITY_Error);
                                                        }
                                                        ListingFile.Append(CurrentFile, LineNumber, Source.StreamName(), Source.LineNumber(), OriginalLine, Source.InMacro());
                                                    }
                                                    catch(ExpressionException Ex)
                                                    {
                                                        throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
                                                    }
                                                    break;
                                                }
                                                case OpCodeEnum::END:
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
                                                        throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error);
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
                                                    case OpCodeTypeEnum::BASIC:
                                                    {
                                                        if(Operands.size() != 0)
                                                            throw AssemblyException("Unexpected operand", AssemblyErrorSeverity::SEVERITY_Error);
                                                        Data.push_back(OpCode->OpCode);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::REGISTER:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Register", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);

                                                        long Register = E.Evaluate(Operands[0]);
                                                        if(OpCode->OpCode == LDN) // Special Case - LDN R0 is overriden by IDL
                                                        {
                                                            if(Register < 1 || Register > 15)
                                                                throw AssemblyException(fmt::format("Register out of range (Expected: $1-$F, got: ${value:X}))", fmt::arg("value", Register)), AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        }
                                                        else
                                                        {
                                                            if(Register < 0 || Register > 15)
                                                                throw AssemblyException(fmt::format("Register out of range (Expected: $0-$F, got: ${value:X}))", fmt::arg("value", Register)), AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        }
                                                        Data.push_back(OpCode->OpCode | Register);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::IMMEDIATE:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Byte", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        long Byte = E.Evaluate(Operands[0]);
                                                        if(Byte > 0xFF && Byte < 0xFF80)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FF, got: ${value:X})", fmt::arg("value", Byte)), AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Byte & 0xFF);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::SHORT_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Short Branch expected single operand", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        long Address = E.Evaluate(Operands[0]);
                                                        if(((ProgramCounter + 1) & 0xFF00) != (Address & 0xFF00))
                                                            throw AssemblyException("Short Branch out of range", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::LONG_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Long Branch expected single operand", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        long Address = E.Evaluate(Operands[0]);
                                                        if(Address < 0 || Address > 0xFFFF)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FFFF, got: ${value:X})", fmt::arg("value", Address)), AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode);
                                                        Data.push_back(Address >> 8);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::INPUT_OUTPUT:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Port", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);

                                                        long Port = E.Evaluate(Operands[0]);
                                                        if(Port == 0 || Port > 7)
                                                            throw AssemblyException("Port out of range (1-7)", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode | Port);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::EXTENDED:
                                                    {
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::EXTENDED_REGISTER:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Register", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);

                                                        long Register = E.Evaluate(Operands[0]);
                                                        if(Register < 0 || Register > 15)
                                                            throw AssemblyException("Register out of range (0-F)", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF | Register);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::EXTENDED_IMMEDIATE:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Expected single operand of type Byte", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        long Byte = E.Evaluate(Operands[0]);
                                                        if(Byte > 0xFF && Byte < 0xFF80)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FF, got :${value:X})", fmt::arg("value", Byte)), AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        Data.push_back(Byte & 0xFF);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::EXTENDED_SHORT_BRANCH:
                                                    {
                                                        if(Operands.size() != 1)
                                                            throw AssemblyException("Short Branch expected single operand", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        long Address = E.Evaluate(Operands[0]);
                                                        if(((ProgramCounter + 2) & 0xFF00) != (Address & 0xFF00))
                                                            throw AssemblyException("Short Branch out of range", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        Data.push_back(OpCode->OpCode >> 8);
                                                        Data.push_back(OpCode->OpCode & 0xFF);
                                                        Data.push_back(Address & 0xFF);
                                                        break;
                                                    }
                                                    case OpCodeTypeEnum::EXTENDED_REGISTER_IMMEDIATE16:
                                                    {
                                                        if(Operands.size() != 2)
                                                            throw AssemblyException("Expected Register and Immediate operands", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        long Register = E.Evaluate(Operands[0]);
                                                        if(Register > 15)
                                                            throw AssemblyException("Register out of range (0-F)", AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
                                                        long Address = E.Evaluate(Operands[1]);
                                                        if(Address < -32768 || Address > 0xFFFF)
                                                            throw AssemblyException(fmt::format("Operand out of range (Expteced: $0-$FFFF, got :${value:X})", fmt::arg("value", Address)), AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
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
                                                throw AssemblyException(Ex.what(), AssemblyErrorSeverity::SEVERITY_Error, OpCode->OpCodeType);
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
                        throw AssemblyException("#if Nesting Error or missing #endif", AssemblyErrorSeverity::SEVERITY_Warning);
                    break;
                case 2:
                    break;
                case 3:
                {
                    // Check for END statement
                    if(!EntryPoint.has_value())
                        throw AssemblyException("END Statement is missing", AssemblyErrorSeverity::SEVERITY_Warning);

                    // Check for un-used non-static SUBROUTINEs and reset to Pass 2 if found

                    if(Errors.count(AssemblyErrorSeverity::SEVERITY_Error) == 0)
                    {
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

                            // Clear Sub Symbol Tables
                            for(auto T = SubTables.begin(); T != SubTables.end(); )
                                if(MainTable.Symbols[T->first].RefCount == 0 && ! T->second.Static)
                                {
                                    TotelOptimisedBytes += T->second.CodeSize;
                                    T = SubTables.erase(T);
                                }
                                else
                                    T++;

                            // Clear Master Symbol Table (Except hidden symbols, i.e. R0-F & P1-7)
                            for(auto it = MainTable.Symbols.begin(); it != MainTable.Symbols.end(); )
                            {
                                if(!it->second.HideFromSymbolTable)
                                    it = MainTable.Symbols.erase(it);
                                else
                                    ++it;
                            }

                            for(auto& Table : SubTables)
                                Table.second.Symbols.clear();

                            //SubTables.clear();

                            // Reset Listing File
                            ListingFile.Reset();
                            break;
                        }
                    }

                    // Check for overlapping code
                    int Overlap = 0;
                    for(auto &Code1 : Code)
                    {
                        uint16_t Start1 = Code1.first;

                        for(auto &Code2 : Code)
                        {
                            if(Code1 != Code2)
                            {
                                uint16_t Start2 = Code2.first;
                                uint16_t End2 = Code2.first + Code2.second.size();
                                if (Start1 >= Start2 && Start1 < End2)
                                    Overlap++;
                            }
                        }
                    }
                    if(Overlap > 0)
                        throw AssemblyException("Code blocks overlap", AssemblyErrorSeverity::SEVERITY_Warning);
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

    int TotalWarnings = Errors.count(AssemblyErrorSeverity::SEVERITY_Warning);
    int TotalErrors = Errors.count(AssemblyErrorSeverity::SEVERITY_Error);

    fmt::println("");
    fmt::println("{count:4} Padding Bytes Added (for ALIGNed SUBROUTINES)", fmt::arg("count", TotalPadBytes));
    fmt::println("{count:4} Bytes Optimised Out (unreferenced SUBROUTINES)", fmt::arg("count", TotelOptimisedBytes));
    fmt::println("");
    fmt::println("{count:4} Warnings",     fmt::arg("count", TotalWarnings));
    fmt::println("{count:4} Errors",       fmt::arg("count", TotalErrors));
    fmt::println("");

    // If no Errors, then write the binary output
    if(TotalErrors == 0)
    {
        BinaryWriter *Output;
        for(auto& Format : BinMode)
        {
            switch(Format)
            {
                case OutputFormatEnum::INTEL_HEX:
                {
                    Output = new BinaryWriter_IntelHex(FileName, "hex");
                    break;
                }
                case OutputFormatEnum::IDIOT4:
                {
                    Output = new BinaryWriter_Idiot4(FileName, "idiot");
                    break;
                }
                case OutputFormatEnum::ELFOS:
                {
                    Output = new BinaryWriter_ElfOS(FileName, "elfos");
                    break;
                }
                case OutputFormatEnum::BIN:
                {
                    Output = new BinaryWriter_Binary(FileName, "bin");
                    break;
                }
            }
            Output->Write(Code, EntryPoint);
            delete Output;
        }
    }

    return TotalErrors == 0 && TotalWarnings == 0;
}

//!
//! \brief ExpandTokens
//! \param Line
//! \param Label
//! \param OpCode
//! \param Operands
//!
//! Expand the source line into Label, OpCode, Operands
const std::optional<OpCodeSpec> Assembler::ExpandTokens(const std::string& Line, std::string& Label, std::string& Mnemonic, std::vector<std::string>& OperandList)
{
    std::smatch MatchResult;
    if(regex_match(Line, MatchResult, std::regex(R"(^(((\w+):?\s*)|\s+)((\w+)(\s+(.*))?)?$)"))) // Label{:} OpCode Operands
    {
        // Extract Label, OpCode and Operands
        std::string Operands;
        Label = MatchResult[3];
        ToUpper(Label);
        if(!Label.empty() && !regex_match(Label, std::regex(R"(^[A-Z_][A-Z0-9_]*$)")))
            throw AssemblyException(fmt::format("Invalid Label: '{Label}'", fmt::arg("Label", Label)), AssemblyErrorSeverity::SEVERITY_Error);
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
            OpCode = { MACROEXPANSION, OpCodeTypeEnum::PSEUDO_OP, CPUTypeEnum::CPU_1802 };
        }
        return OpCode;
    }
    else
        throw AssemblyException("Unable to parse line", AssemblyErrorSeverity::SEVERITY_Error);
}

//!
//! \brief ExpandMacro
//! Apply the Operands to the Macro Arguments, and return the Expanded string with the operands replaced.
//! \param Definition
//! \param Operands
//! \return
//!
void Assembler::ExpandMacro(const Macro& Definition, const std::vector<std::string>& Operands, std::string& Output)
{
    if(Definition.Arguments.size() != Operands.size())
        throw AssemblyException(fmt::format("Incorrect number of arguments passed to macro. Received {In}, Expected {Out}", fmt::arg("In", Operands.size()), fmt::arg("Out", Definition.Arguments.size())), AssemblyErrorSeverity::SEVERITY_Error);

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
//! \brief GetFileName
//! Extract the filename component from an @"Filename" DB parameter
//! \param Operand
//! \return
//!
std::string Assembler::GetFileName(std::string Operand)
{
    std::smatch MatchResult;
    if(regex_match(Operand, MatchResult, std::regex(R"(^\"(.+)\"$)")))
        return MatchResult[1];
    else
        throw AssemblyException("Not Supported", AssemblyErrorSeverity::SEVERITY_Error);
}

//!
//! \brief StringToByteVector
//! Scan the passed quoted string, return a byte array, resolving escaped special characters
//! Assumes first character is double quote, and skips it.
//! \param Operand
//! \param Data
//!
void Assembler::StringToByteVector(const std::string& Operand, std::vector<uint8_t>& Data)
{
    int Len = 0;
    bool QuoteClosed = false;
    for(int i = 1; i< Operand.size(); i++)
    {
        if(Operand[i] == '\"')
        {
            if(i != Operand.size() - 1)
                throw AssemblyException("Error parsing string constant", AssemblyErrorSeverity::SEVERITY_Error);
            else
            {
                QuoteClosed = true;
                break;
            }
        }
        if(Operand[i] == '\\')
        {
            if(i >= Operand.size() - 2)
                throw AssemblyException("Incomplete escape sequence at end of string constant", AssemblyErrorSeverity::SEVERITY_Error);
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
                    throw AssemblyException("Unrecognised escape sequence in string constant", AssemblyErrorSeverity::SEVERITY_Error);
                    break;
            }
        }
        else
            Data.push_back(Operand[i]);
        Len++;
    }
    if(Len == 0)
        throw AssemblyException("String constant is empty", AssemblyErrorSeverity::SEVERITY_Error);
    if(!QuoteClosed)
        throw AssemblyException("unterminated string constant", AssemblyErrorSeverity::SEVERITY_Error);
}

//!
//! \brief StringListToVector
//! \param Input
//! \param Output
//! \param Delimiter
//!
void Assembler::StringListToVector(std::string& Input, std::vector<std::string>& Output, char Delimiter)
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
int Assembler::AlignFromSize(int Size)
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
//! \brief SetAlignFromKeyword
//! Return an integer representation from an alignment keyword
//! \param Alignment
//! \param Align
//! \return
//!
bool Assembler::SetAlignFromKeyword(std::string Alignment, long& Align)
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

//!
//! \brief GetAlignExtraBytes
//! Calculate the number of bytes to add to the program counter to align on an 'align' byte boundary
//! \param ProgramCounter
//! \param Align
//! \return
//!
int Assembler::GetAlignExtraBytes(int ProgramCounter, int Align)
{
    if(Align == 0)
        return 0;
    else
        return (Align - ProgramCounter % Align) % Align;
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
void Assembler::PrintError(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const std::string& Line, const std::string& Message, const AssemblyErrorSeverity Severity, const bool InMacro)
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

//!
//! \brief PrintError
//! Print an error message
//! \param Message
//! \param Severity
//!
void Assembler::PrintError(const std::string& Message, AssemblyErrorSeverity Severity)
{
    fmt::println("***************{severity:*>15}: {message}",
                 fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                 fmt::arg("message", Message));
}
