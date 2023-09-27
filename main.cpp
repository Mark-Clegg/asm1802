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

DefineMap GlobalDefines;   // global #defines persist for all files following on the command line

bool assemble(const std::string&, bool ListingEnabled, bool DumpSymbols);

void PrintError(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity);
void PrintError(const std::string& Message, AssemblyErrorSeverity Severity);
void PrintSymbols(const std::string & Name, const SymbolTable& Table);

#if DEBUG
void DumpCode(const std::map<uint16_t, std::vector<uint8_t>>& Code);
#endif

int main(int argc, char **argv)
{

    option longopts[] = {
        { "define", required_argument, 0, 'D' },
        { "undefine", required_argument, 0, 'U' },
        { "list", no_argument, 0, 'L' },
        { "symbols", no_argument, 0 , 'S' },
        { 0,0,0,0 }
    };

    bool Listing = false;
    bool Symbols = false;
    int FileCount = 0;
    int FilesAssembled = 0;
    while (1) {

        const int opt = getopt_long(argc, argv, "-D:U:LS", longopts, 0);

        if (opt == -1) {
            break;
        }

        switch (opt) {
        case 1:
            try
            {
                if(assemble(optarg, Listing, Symbols))
                    FilesAssembled++;
                FileCount++;
            }
            catch (AssemblyException Error)
            {
                fmt::print("** Error: Unable to open/read file: {message}\n", fmt::arg("message", Error.Message));
            }
            break;

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
            if(GlobalDefines.contains(key))
                fmt::print("** Warning: Macro redeclared: {key}\n", fmt::arg("key", key));
            GlobalDefines[key] = value;
            break;
        }

        case 'U':
            GlobalDefines.erase(optarg);
            break;

        case 'L':
            Listing = true;
            break;

        case 'S':
            Symbols = true;
            break;

        default:
            return 1;
            fmt::print("Error\n");
        }
    }

    while (optind < argc)
    {
        try
        {
            if(assemble(argv[optind++], Listing, Symbols))
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
bool assemble(const std::string& FileName, bool ListingEnabled, bool DumpSymbols)
{
    fmt::print("Assembling: {filename}\n", fmt::arg("filename", FileName));
    
    SymbolTable MainTable;
    std::map<std::string, SymbolTable> SubTables;
    std::map<uint16_t, std::vector<uint8_t>> Code = {{ 0, {}}};
    std::map<uint16_t, std::vector<uint8_t>>::iterator CurrentCode = Code.begin();

    SourceCodeReader Source;
    ErrorTable Errors;
    ListingFileWriter ListingFile(Source, FileName, Errors, ListingEnabled);

    for(int Pass = 1; Pass <= 3 && Errors.count(SEVERITY_Error) == 0; Pass++)
    {
        SymbolTable* CurrentTable = &MainTable;
        uint16_t ProgramCounter = 0;
        bool InSub = false;
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
                                if(Defines.contains(key))
                                    throw AssemblyException("Duplicate definition", SEVERITY_Warning);
                                Defines[key]=value;
                                break;
                            }
                            case PP_undef:
                            {
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
                                Source.IncludeFile(Expression);
                                break;
                            }
                            case PP_list:
                            {
                                if(Expression == "on")
                                    ListingFile.Enabled = true;
                                else if(Expression == "off")
                                    ListingFile.Enabled = false;
                                else
                                    throw AssemblyException("#list must specify 'on' or 'off'", SEVERITY_Warning);
                                break;
                            }
                            case PP_symbols:
                            {
                                if(Expression == "on")
                                    DumpSymbols = true;
                                else if(Expression == "off")
                                    DumpSymbols = false;
                                else
                                    throw AssemblyException("#symbols must specify 'on' or 'off'", SEVERITY_Warning);
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
                                case 1: // Calculate the size of relocatable subroutines
                                {
                                    if(OpCode)
                                    {
                                        switch(OpCode.value().OpCode)
                                        {
                                        case SUB:
                                            if(Label.empty())
                                                throw AssemblyException("SUBROUTINE requires a Label", SEVERITY_Error);

                                            if(SubTables.find(Label) != SubTables.end())
                                                throw AssemblyException(fmt::format("Subroutine '{Label}' is already defined", fmt::arg("Label", Label)), SEVERITY_Error);

                                            if(!CurrentTable->Master)
                                                throw AssemblyException("SUBROUTINEs cannot be nested", SEVERITY_Error);

                                            if(Operands.empty())
                                            {
                                                SubTables.insert(std::pair<std::string, SymbolTable>(Label, SymbolTable(false, ProgramCounter)));
                                                CurrentTable = &SubTables[Label];
                                            }
                                            else
                                            {
                                                ToUpper(Operands[0]);
                                                if(Operands[0] == "RELOCATABLE")
                                                {
                                                    SubTables.insert(std::pair<std::string, SymbolTable>(Label, SymbolTable(true)));
                                                    CurrentTable = &SubTables[Label];
                                                    ProgramCounter = 0;
                                                }
                                                else
                                                    throw AssemblyException(fmt::format("Unrecognised operand '{Operand}'", fmt::arg("Operand", Operands[0])), SEVERITY_Error);
                                            }
                                            break;
                                        case ENDSUB:
                                        {
                                            if(CurrentTable->Relocatable)
                                            {
                                                // throw AssemblyException("ENDSUB without matching SUB", SEVERITY_Error);
                                                CurrentTable->CodeSize = ProgramCounter;
                                            }
                                            CurrentTable = &MainTable;
                                            break;
                                        }
                                        default: // All Native Opcodes handled here.
                                        {
                                            if(OpCode && OpCode.value().OpCodeType != PSEUDO_OP)
                                                ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                            break;
                                        }
                                        }
                                    }
                                    break;
                                }
                                case 2: // Generate Symbol Tables
                                {
                                    if(!Label.empty())
                                    {
                                        if(CurrentTable->Symbols.find(Label) == CurrentTable->Symbols.end())
                                            CurrentTable->Symbols[Label] = ProgramCounter;
                                        else
                                        {
                                            auto& Symbol = CurrentTable->Symbols[Label];
                                            if(Symbol.has_value())
                                                throw AssemblyException(fmt::format("Label '{Label}' is already defined", fmt::arg("Label", Label)), SEVERITY_Error);
                                            Symbol = ProgramCounter;
                                        }
                                    }

                                    // subroutine label
                                    //    if not in symbol table, add and set value and set public flag
                                    //    if in symbol table and no value, and not extern - set value and set public
                                    //    else error

                                    if(OpCode)
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
                                            int Value = E.Evaluate(Operands[0]);
                                            CurrentTable->Symbols[Label] = Value;
                                            break;
                                        }
                                        case SUB:
                                        {
                                            InSub = true;
                                            if(Operands.empty())
                                            {
                                                CurrentTable = &SubTables[Label];
                                            }
                                            else
                                            {
                                                ToUpper(Operands[0]);
                                                if(Operands[0] == "RELOCATABLE")
                                                {
                                                    CurrentTable = &SubTables[Label];
                                                    ProgramCounter = ProgramCounter + AlignFromSize(CurrentTable->CodeSize) - ProgramCounter % AlignFromSize(CurrentTable->CodeSize);
                                                }
                                                else
                                                    throw AssemblyException(fmt::format("Unrecognised operand '{Operand}'", fmt::arg("Operand", Operands[0])), SEVERITY_Error);
                                            }
                                            break;
                                        }
                                        case ENDSUB:
                                        {
                                            if(!InSub)
                                                throw AssemblyException("ENDSUB without matching SUB", SEVERITY_Error);
                                            InSub = false;
                                            CurrentTable = &MainTable;
                                            break;
                                        }
                                        case ORG:
                                        {
                                            if(!CurrentTable->Master)
                                                throw AssemblyException("ORG Cannot be used in a SUBROUTINE", SEVERITY_Error);

                                            if(Operands.size() != 1)
                                                throw AssemblyException("ORG Requires a single argument <address>", SEVERITY_Error);

                                            ExpressionEvaluator E(MainTable, ProgramCounter);
                                            ProgramCounter = E.Evaluate(Operands[0]);

                                            if(!Label.empty())
                                                CurrentTable->Symbols[Label] = ProgramCounter;

                                            break;
                                        }
                                        default: // All Native Opcodes handled here.
                                        {
                                             if(OpCode && OpCode.value().OpCodeType != PSEUDO_OP)
                                                ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                            break;
                                        }
                                        }
                                    }
                                    break;
                                }
                                case 3: // Generate Code
                                {
                                    if(OpCode)
                                    {
                                        switch(OpCode.value().OpCode)
                                        {
                                        case SUB:
                                        {
                                            CurrentTable = &SubTables[Label];
                                            if(CurrentTable->Relocatable)
                                            {
                                                int Align = AlignFromSize(CurrentTable->CodeSize);

                                                if(ProgramCounter % Align > 0)
                                                    ProgramCounter = ProgramCounter + Align - ProgramCounter % Align;
                                            }
                                            InSub = true;
                                            CurrentCode = Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {})).first;
                                            ListingFile.Append();
                                            break;
                                        }
                                        case ENDSUB:
                                        {
                                            if(!InSub)
                                                throw AssemblyException("ENDSUB without matching SUB", SEVERITY_Error);
                                            InSub = false;
                                            CurrentTable = &MainTable;
                                            ListingFile.Append();
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
                                        default:
                                        {
                                            if(OpCode && OpCode.value().OpCodeType != PSEUDO_OP)
                                            {
                                                std::vector<std::uint8_t> Data;
                                                ExpressionEvaluator E(MainTable, ProgramCounter);
                                                if(CurrentTable->Relocatable)
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
                                                    if(Byte > 255)
                                                        throw AssemblyException("Immediate operand out of range (0-FF)", SEVERITY_Error);
                                                    Data.push_back(OpCode->OpCode);
                                                    Data.push_back(Byte);
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
                                                    if(Byte > 255)
                                                        throw AssemblyException("Immediate operand out of range (0-FF)", SEVERITY_Error);
                                                    Data.push_back(OpCode->OpCode >> 8);
                                                    Data.push_back(OpCode->OpCode & 0xFF);
                                                    Data.push_back(Byte);
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
                                            else
                                                ListingFile.Append();
                                            break;
                                            }
                                        }
                                    }
                                }
                                }
                            }
                            catch (AssemblyException Ex)
                            {
                                PrintError(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                                Errors.Push(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                                if (Pass == 3) {
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
                    PrintError(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                    Errors.Push(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                }
            }
            if(IfNestingLevel != 0)
                throw AssemblyException("#if Nesting Error or missing #endif", SEVERITY_Warning, false);

            // Check for overlapping code
            if(Pass == 3)
            {
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
                    throw AssemblyException("Code blocks ovelap", SEVERITY_Warning, false);
            }

        }
        catch (AssemblyException Ex)
        {
            if(Ex.ShowLine)
            {
                PrintError(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                //Errors.Push(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
            }
            else
            {
                PrintError(Ex.Message, Ex.Severity);
                //Errors.Push(Ex.Message, Ex.Severity);
            }
        }
    }

    if(DumpSymbols)
    {
        fmt::print("\n");

        PrintSymbols("Global Symbols", MainTable);
        ListingFile.AppendSymbols("Global Symbols", MainTable);
        for(auto& Table : SubTables)
        {
            PrintSymbols(Table.first, Table.second);
            ListingFile.AppendSymbols(Table.first, Table.second);
        }
    }

    int TotalWarnings = Errors.count(SEVERITY_Warning);
    int TotalErrors = Errors.count(SEVERITY_Error);

    fmt::print("\n");
    fmt::print("{count:4} Warnings\n",     fmt::arg("count", TotalWarnings));
    fmt::print("{count:4} Errors\n",       fmt::arg("count", TotalErrors));
    fmt::print("\n");

#if DEBUG
    DumpCode(Code);
#endif

    return TotalErrors == 0 && TotalWarnings == 0;
}

#if DEBUG
void DumpCode(const std::map<uint16_t, std::vector<uint8_t>>& Code)
{
    fmt::print("Static Code\n");
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
    try // Source may not contain anything...
    {
        fmt::print("[{filename:22}({linenumber:5})] {line}\n",
                   fmt::arg("filename", " "),
                   fmt::arg("linenumber", " "),
                   fmt::arg("line", " ")
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


//!
//! \brief PrintSymbols
//! \param Name
//! \param Symbols
//!
//! Print the given Symbol Table
//!
void PrintSymbols(const std::string& Name, const SymbolTable& Blob)
{
    std::string Title;
    if(Blob.Relocatable)
        Title = Name + " (Relocatable)";
    else
        Title = Name;
    fmt::print("{Title:-^116}\n", fmt::arg("Title", Title));

    int c = 0;
    for(auto& Symbol : Blob.Symbols)
    {
        fmt::print("{Name:15} ", fmt::arg("Name", Symbol.first));
        if(Symbol.second.has_value())
        fmt::print("{Address:04X}", fmt::arg("Address", Symbol.second.value()));
        if(++c % 5 == 0)
            fmt::print("\n");
        else
            fmt::print("    ");
    }
    fmt::print("\n");
}
