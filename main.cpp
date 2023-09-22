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
#include "blob.h"
#include "utils.h"

namespace fs = std::filesystem;

DefineMap GlobalDefines;   // global #defines persist for all files following on the command line

bool assemble(const std::string&, bool ListingEnabled, bool DumpSymbols);

void PrintError(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity);
void PrintSymbols(const std::string & Name, const blob& Table);

#if DEBUG
void DumpCode(const blob &Blob);
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
    
    blob MainBlob;
    std::map<std::string, blob> SecondaryBlob;
    std::stack<uint16_t> ProgramCounterStack;

    SourceCodeReader Source;
    ErrorTable Errors;
    ListingFileWriter ListingFile(Source, FileName, Errors, ListingEnabled);

    for(int Pass = 1; Pass <= 2 && Errors.count(SEVERITY_Error) == 0; Pass++)
    {
        blob* CurrentBlob = &MainBlob;
        uint16_t ProgramCounter = 0;
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
                            if (Pass == 2)
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
                                        if (Pass == 2) // Add terminating directive from skiplines to listing output
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
                                        if (Pass == 2) // Add terminating directive from skiplines to listing output
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
                                    if (Pass == 2) // Add terminating directive from skiplines to listing output
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
                                    case 1:
                                    {
                                        if(!Label.empty())
                                        {
                                        if(CurrentBlob->Symbols.find(Label) == CurrentBlob->Symbols.end())
                                            CurrentBlob->Symbols[Label].Value = ProgramCounter;
                                            else
                                            {
                                            auto& Symbol = CurrentBlob->Symbols[Label];
                                                if(Symbol.Extern)
                                                    throw AssemblyException(fmt::format("Cannot declare label '{Label}' here as it was previously declared as extern", fmt::arg("Label", Label)), SEVERITY_Error);
                                                if(Symbol.Value.has_value())
                                                    throw AssemblyException(fmt::format("Label '{Label}' is already defined", fmt::arg("Label", Label)), SEVERITY_Error);
                                                Symbol.Value = ProgramCounter;
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
                                            case EXTERN:
                                            {
                                                // if not in symbol table add and set extern flag
                                                // else error
                                                if(Operands.size() == 1)
                                                {
                                                    if(CurrentBlob->Master)
                                                    {
                                                        if(CurrentBlob->Symbols.find(Operands[0]) == CurrentBlob->Symbols.end())
                                                            CurrentBlob->Symbols[Operands[0]].Extern = true;
                                                        else
                                                            throw AssemblyException(fmt::format("Label '{Label}' is already defined", fmt::arg("Label", Label)), SEVERITY_Error);
                                                    }
                                                    else
                                                        throw AssemblyException("'extern' can only be used at top level scope", SEVERITY_Error);
                                                }
                                                else
                                                    throw AssemblyException("Too many operands", SEVERITY_Error);
                                                break;
                                            }
                                            case PUBLIC:
                                            {
                                                // if in symbol table, set public flag
                                                // if not in symbol table, add and set public flag
                                                if(Operands.size() == 1)
                                                {
                                                    if(CurrentBlob->Master)
                                                        CurrentBlob->Symbols[Operands[0]].Public = true;
                                                    else
                                                        throw AssemblyException("'public' can only be used at top level scope", SEVERITY_Error);
                                                }
                                                else
                                                    throw AssemblyException("Too many operands", SEVERITY_Error);
                                                break;
                                            }
                                            case SUB:
                                            {
                                                if(Label.empty())
                                                    throw AssemblyException("SUBROUTINE requires a Label", SEVERITY_Error);

                                                if(SecondaryBlob.find(Label) != SecondaryBlob.end())
                                                    throw AssemblyException(fmt::format("Subroutine '{Label}' is already defined", fmt::arg("Label", Label)), SEVERITY_Error);

                                                if(!CurrentBlob->Master)
                                                    throw AssemblyException("SUBROUTINEs cannot be nested", SEVERITY_Error);

                                                if(Operands.empty())
                                                {
                                                    CurrentBlob->Symbols[Label].Public = true;
                                                    SecondaryBlob.insert(std::pair<std::string, blob>(Label, blob(false, ProgramCounter)));
                                                    CurrentBlob = &SecondaryBlob[Label];
                                                }
                                                else
                                                {
                                                    ToUpper(Operands[0]);
                                                    if(Operands[0] == "RELOCATABLE")
                                                    {
                                                        CurrentBlob->Symbols[Label].Public = true;
                                                        SecondaryBlob.insert(std::pair<std::string, blob>(Label, blob(true)));
                                                        CurrentBlob = &SecondaryBlob[Label];
                                                        ProgramCounterStack.push(ProgramCounter);
                                                        ProgramCounter = 0;
                                                    }
                                                    else
                                                        throw AssemblyException(fmt::format("Unrecognised operand '{Operand}'", fmt::arg("Operand", Operands[0])), SEVERITY_Error);
                                                }
                                                break;
                                            }
                                            case ENDSUB:
                                            {
                                                if(CurrentBlob->Relocatable)
                                                {
                                                    if(ProgramCounterStack.empty())
                                                        throw AssemblyException("ENDSUB without matching SUB", SEVERITY_Error);
                                                    ProgramCounter = ProgramCounterStack.top();
                                                    ProgramCounterStack.pop();
                                                }
                                                CurrentBlob = &MainBlob;
                                                break;
                                            }
                                            case ORG:
                                            {
                                                if(!CurrentBlob->Master)
                                                    throw AssemblyException("ORG Cannot be used in a SUBROUTINE", SEVERITY_Error);

                                                if(Operands.size() != 1)
                                                    throw AssemblyException("ORG Requires a single argument <address>", SEVERITY_Error);

                                                ExpressionEvaluator E(MainBlob);
                                                ProgramCounter = E.Evaluate(Operands[0]);

                                                if(!Label.empty())
                                                    CurrentBlob->Symbols[Label].Value = ProgramCounter;

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
                                    case 2:
                                    {
                                        if(OpCode)
                                        {
                                            switch(OpCode.value().OpCode)
                                            {
                                            case SUB:
                                            {
                                                if(SecondaryBlob[Label].Relocatable)
                                                {
                                                    ProgramCounterStack.push(ProgramCounter);
                                                    ProgramCounter = 0;
                                                }
                                                CurrentBlob = &SecondaryBlob[Label];
                                                ListingFile.Append();
                                                break;
                                            }
                                            case ENDSUB:
                                            {
                                                if(CurrentBlob->Relocatable)
                                                {
                                                    if(ProgramCounterStack.empty())
                                                        throw AssemblyException("ENDSUB without matching SUB", SEVERITY_Error);
                                                    ProgramCounter = ProgramCounterStack.top();
                                                    ProgramCounterStack.pop();
                                                }
                                                CurrentBlob = &MainBlob;
                                                ListingFile.Append();
                                                break;
                                            }
                                            case ORG:
                                            {
                                                ExpressionEvaluator E(MainBlob);
                                                ProgramCounter = E.Evaluate(Operands[0]);

                                                auto InsertResult = CurrentBlob->Code.insert(std::pair<uint16_t, std::vector<uint8_t>>(ProgramCounter, {}));
                                                CurrentBlob->CurrentCode = InsertResult.first;

                                                ListingFile.Append();
                                                break;
                                            }
                                            default:
                                            {
                                                if(OpCode && OpCode.value().OpCodeType != PSEUDO_OP)
                                                {
                                                    std::vector<std::uint8_t> Data;
                                                    ExpressionEvaluator E(MainBlob);
                                                    if(CurrentBlob->Relocatable)
                                                        E.AddLocalSymbols(CurrentBlob);

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
                                                        if((ProgramCounter + 1) & 0xFF00 != Address & 0xFF00)
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
                                                        if((ProgramCounter + 2) & 0xFF00 != Address & 0xFF00)
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
                                                    }

                                                    if(CurrentBlob->Relocatable)
                                                        CurrentBlob->CurrentCode->second.insert(CurrentBlob->CurrentCode->second.end(), Data.begin(), Data.end());
                                                    else
                                                        MainBlob.CurrentCode->second.insert(MainBlob.CurrentCode->second.end(), Data.begin(), Data.end());

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
                                if (Pass == 2) {
                                    ListingFile.Append();
                                }
                            }

                        }
                    }
                    else // Empty line
                        if (Pass == 2)
                            ListingFile.Append();
                }
                catch (AssemblyException Ex)
                {
                    PrintError(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                    Errors.Push(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
                }
            }
            if(IfNestingLevel != 0)
                throw AssemblyException("#if Nesting Error or missing #endif", SEVERITY_Warning);
        }
        catch (AssemblyException Ex)
        {
            PrintError(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
            Errors.Push(Source.getFileName(), Source.getLineNumber(), Source.getLastLine(), Ex.Message, Ex.Severity);
        }
    }


    if(DumpSymbols)
    {
        fmt::print("\n");

        PrintSymbols("Global Symbols", MainBlob);
        ListingFile.AppendSymbols("Global Symbols", MainBlob);
        for(auto& Table : SecondaryBlob)
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
    fmt::print("Static Code\n");
    DumpCode(MainBlob);

    for(const auto &NamedBlob : SecondaryBlob)
    {
        int Bytes = 0;
        for(const auto &Blob : NamedBlob.second.Code)
            Bytes += Blob.second.size();

        if (Bytes > 0)
        {
            fmt::print("\nRelocatable Code ({Name})\n", fmt::arg("Name", NamedBlob.first));
            DumpCode(NamedBlob.second);
        }
    }
    fmt::print("\n");
#endif

    return TotalErrors == 0 && TotalWarnings == 0;
}

#if DEBUG
void DumpCode(const blob& Blob)
{
    for(auto &Segment : Blob.Code)
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

//!
//! \brief PrintSymbols
//! \param Name
//! \param Symbols
//!
//! Print the given Symbol Table
//!
void PrintSymbols(const std::string& Name, const blob& Blob)
{
    std::string Title;
    if(Blob.Relocatable)
        Title = Name + " (Relocatable)";
    else
        Title = Name;
    fmt::print("{Title:-^108}\n", fmt::arg("Title", Title));

    int c = 0;
    for(auto& Symbol : Blob.Symbols)
    {
        fmt::print("{Name:15} ", fmt::arg("Name", Symbol.first));
        if(Symbol.second.Extern)
            fmt::print("External");
        else
        {
            if(Symbol.second.Value.has_value())
            fmt::print("{Address:04X} {Public:3}",
                           fmt::arg("Address", Symbol.second.Value.value()),
                       fmt::arg("Public", Symbol.second.Public ? "Pub" : "   "));
        else
            fmt::print("---- {Public:3}", fmt::arg("Public", Symbol.second.Public ? "Pub" : "   "));
        }
        if(++c % 4 == 0)
            fmt::print("\n");
        else
            fmt::print("    ");
    }
    fmt::print("\n");
}
