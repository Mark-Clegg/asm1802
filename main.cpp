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
#include "exceptions.h"
#include "listingfilewriter.h"
#include "opcode.h"
#include "sourcecodereader.h"
#include "utils.h"

namespace fs = std::filesystem;

DefineMap GlobalDefines;   // global #defines persist for all files following on the command line

bool assemble(const std::string&, bool ListingEnabled);

void Error(SourceCodeReader& Source, const std::string& Message, const AssemblyErrorSeverity Severity);

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
            catch (AssemblyError Error)
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
        catch (AssemblyError Error)
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

    for(int Pass = 1; Pass < 3; Pass++)
    {
        AssemblyError::ResetErrorCounters();
        SourceCodeReader Source;
        try
        {
            fmt::print("Pass {pass}\n", fmt::arg("pass", Pass));

            // Setup Source File stack
            Source.IncludeFile(FileName);

            ListingFileWriter ListingFile(Source, ListingEnabled);

            // Initialise Defines from Global Defines
            DefineMap Defines(GlobalDefines);   // Defines, initialised from Global Defines

            // Setup stack of #if results
            int IfIndentLevel = 0;

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
                                    throw AssemblyError("Duplicate definition", SEVERITY_Warning, Source.getFileName(), Source.getLineNumber());
                                Defines[key]=value;
                                break;
                            }
                            case PP_undef:
                            {
                                if(Defines.contains(Expression))
                                    Defines.erase(Expression);
                                else
                                    throw AssemblyError("Not defined", SEVERITY_Warning, Source.getFileName(), Source.getLineNumber());
                                break;
                            }
                            case  PP_if:
                            {
                                IfIndentLevel++;
                                throw AssemblyError("Not Implemented - Assuming True", SEVERITY_Warning, Source.getFileName(), Source.getLineNumber());
                                break;
                            }
                            case PP_ifdef:
                            {
                                IfIndentLevel++;
                                if(!Defines.contains(Expression))
                                {
                                    if(SkipLines(Source, OriginalLine) == PP_endif)
                                    {
                                        if (Pass == 2) // Add terminating directive from skiplines to listing output
                                            ListingFile.Append();
                                        IfIndentLevel--;
                                    }
                                }
                                break;
                            }
                            case PP_ifndef:
                            {
                                IfIndentLevel++;
                                if(Defines.contains(Expression))
                                {
                                    if(SkipLines(Source, OriginalLine) == PP_endif)
                                    {
                                        if (Pass == 2) // Add terminating directive from skiplines to listing output
                                            ListingFile.Append();
                                        IfIndentLevel--;
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
                                    IfIndentLevel--;
                                }
                                break;
                            }
                            case PP_endif:
                            {
                                IfIndentLevel--;
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
                                    throw AssemblyError("#list must specify 'on' or 'off'", SEVERITY_Warning);
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
                                std::optional<OPCODE> MachineWord = ExpandTokens(Line, Label, Mnemonic, Operands);

                                std::vector<std::uint8_t> Data;

                                Data.push_back(0xf8);
                                Data.push_back(0x01);

                                if (Pass == 2)
                                    ListingFile.Append(0x1f3, Data);
                            }
                            catch (AssemblyError Ex)
                            {
                                Error(Source, Ex.Message, Ex.Severity);
                                if (Pass == 2) {
                                    ListingFile.Append();
                                    ListingFile.AppendError(Ex.Message, Ex.Severity);
                                }
                            }

                        }
                    }
                    else // Empty line
                        if (Pass == 2)
                            ListingFile.Append();
                }
                catch (AssemblyError Ex)
                {
                    Error(Source, Ex.Message, Ex.Severity);
                    if(Pass == 2)
                    {
                        ListingFile.Append();
                        ListingFile.AppendError(Ex.Message, Ex.Severity);
                    }
                }
            }
            if(IfIndentLevel != 0)
                throw AssemblyError("#if Nesting Error or missing #endif", SEVERITY_Warning);
        }
        catch (AssemblyError Ex)
        {
            Error(Source, Ex.Message, Ex.Severity);
            //if(Pass == 2)
            //    ListingFile.AppendError(Ex.Message, Ex.Severity);
        }
    }

    fmt::print("{count:4} Warnings\n",     fmt::arg("count", AssemblyError::WarningCount));
    fmt::print("{count:4} Errors\n",       fmt::arg("count", AssemblyError::ErrorCount));
    fmt::print("{count:4} Fatal Errors\n", fmt::arg("count", AssemblyError::FatalCount));
    fmt::print("\n");

    return AssemblyError::ErrorCount == 0 && AssemblyError::FatalCount == 0;
}

//!
//! \brief Error
//! \param FileName
//! \param LineNumber
//! \param Line
//! \param Message
//! \param Severity
//!
//! Write the current line and error message to stderr
void Error(SourceCodeReader &Source, const std::string& Message, const AssemblyErrorSeverity Severity)
{
    switch(Severity)
    {
    case SEVERITY_Warning:  AssemblyError::WarningCount++; break;
    case SEVERITY_Error:    AssemblyError::ErrorCount++;   break;
    case SEVERITY_Fatal:    AssemblyError::FatalCount++;   break;
    }

    try // Source may not contain anything...
    {
        std::string FileName = fs::path(Source.getFileName()).filename();
        if(FileName.length() > 20)
            FileName = FileName.substr(0, 17) + "...";


        fmt::print("[{filename:22}({linenumber:5})] {line}\n",
                   fmt::arg("filename", FileName),
                   fmt::arg("linenumber", Source.getLineNumber()),
                   fmt::arg("line", Source.getLastLine())
                   );
        fmt::print("***************{severity:*>15}: {message}\n",
                   fmt::arg("severity", " "+AssemblyError::SeverityName.at(Severity)),
                   fmt::arg("message", Message));
    }
    catch(...)
    {
        fmt::print("***************{severity:*>15}: {message}\n",
                   fmt::arg("severity", " "+AssemblyError::SeverityName.at(Severity)),
                   fmt::arg("message", Message));
    }
}
