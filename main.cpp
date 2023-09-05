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
#include "listingfilewriter.h"
#include "opcodetable.h"
#include "sourcecodereader.h"
#include "symbol.h"
#include "utils.h"

namespace fs = std::filesystem;

DefineMap GlobalDefines;   // global #defines persist for all files following on the command line

bool assemble(const std::string&, bool ListingEnabled);

void PrintError(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, AssemblyErrorSeverity Severity);

int main(int argc, char **argv)
{

    option longopts[] = {
        { "define", required_argument, 0, 'D' },
        { "undefine", required_argument, 0, 'U' },
        { "list", no_argument, 0, 'L' },
        { 0,0,0,0 }
    };

    bool Listing = false;
    int FileCount = 0;
    int FilesAssembled = 0;
    while (1) {

        const int opt = getopt_long(argc, argv, "-D:U:L", longopts, 0);

        if (opt == -1) {
            break;
        }

        switch (opt) {
        case 1:
            try
            {
                if(assemble(optarg, Listing))
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

        default:
            return 1;
            fmt::print("Error\n");
        }
    }

    while (optind < argc)
    {
        try
        {
            if(assemble(argv[optind++], Listing))
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
bool assemble(const std::string& FileName, bool ListingEnabled)
{
    fmt::print("Assembling: {filename}\n", fmt::arg("filename", FileName));

    symbolTable MasterSymbolTable(false);
    symbolTable* CurrentScope = &MasterSymbolTable;

    SourceCodeReader Source;
    ErrorTable Errors;
    ListingFileWriter ListingFile(Source, FileName, Errors, ListingEnabled);

    for(int Pass = 1; Pass < 3; Pass++)
    {
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
                                            if(CurrentScope->Table.find(Label) == CurrentScope->Table.end())
                                                CurrentScope->Table[Label].Address = ProgramCounter;
                                            else
                                            {
                                                auto Symbol = CurrentScope->Table[Label];
                                                if(Symbol.Extern)
                                                    throw AssemblyException("Label is already defined as extern", SEVERITY_Error);
                                                if(Symbol.Address.has_value())
                                                    throw AssemblyException("Label is already defined", SEVERITY_Error);
                                                Symbol.Address = ProgramCounter;
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
                                                    if(CurrentScope == &MasterSymbolTable)
                                                    {
                                                        if(CurrentScope->Table.find(Operands[0]) == CurrentScope->Table.end())
                                                            CurrentScope->Table[Operands[0]].Extern = true;
                                                        else
                                                            throw AssemblyException("Label is already defined", SEVERITY_Error);
                                                    }
                                                    else
                                                        throw AssemblyException("'extern' can only be used in top level scope", SEVERITY_Error);
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
                                                    if(CurrentScope == &MasterSymbolTable)
                                                        CurrentScope->Table[Operands[0]].Public = true;
                                                    else
                                                        throw AssemblyException("'public' can only be used in top level scope", SEVERITY_Error);
                                                }
                                                else
                                                    throw AssemblyException("Too many operands", SEVERITY_Error);
                                                break;
                                            }
                                            default: // All Native Opcodes handled here.
                                            {
                                                ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                                break;
                                            }
                                            }
                                        }
                                        break;
                                    }
                                    case 2:
                                    {
                                        std::vector<std::uint8_t> Data;

                                        if(OpCode && OpCode.value().OpCodeType != PSEUDO_OP)
                                        {
                                            for(int i=0; i<OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType); i++)
                                                if(i==0)
                                                    Data.push_back(OpCode->OpCode); // TODO Merge Register/Port arguments if reqd
                                                else
                                                    Data.push_back(0); // TODO push arguments
                                            ListingFile.Append(ProgramCounter, Data);
                                            ProgramCounter += OpCodeTable::OpCodeBytes.at(OpCode->OpCodeType);
                                        }
                                        else
                                            ListingFile.Append();
                                        break;
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

    int TotalWarnings = Errors.count(SEVERITY_Warning);
    int TotalErrors = Errors.count(SEVERITY_Error);

    fmt::print("{count:4} Warnings\n",     fmt::arg("count", TotalWarnings));
    fmt::print("{count:4} Errors\n",       fmt::arg("count", TotalErrors));
    fmt::print("\n");

    return TotalErrors == 0 && TotalWarnings == 0;
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
