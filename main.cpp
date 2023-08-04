#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <getopt.h>
#include "codeblock.h"
#include "definemap.h"
#include "exceptions.h"
#include "listingfilewriter.h"
#include "sourcecodereader.h"
#include "utils.h"

DefineMap GlobalDefines;   // global #defines persist for all files following on the command line

bool assemble(const std::string&, bool ListingEnabled);

void Error(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, const AssemblyErrorSeverity Severity);

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
                std::cerr << "** Error: Unable to open/read file: " << Error.Message << std::endl;
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
                std::cerr << "** Warning: Macro redeclared: " << key << std::endl;
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
            std::cout << "Error" << std::endl;
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
            std::cerr << "** Error opening/reading file: " << Error.Message << std::endl;
        }
    }

    std::cout << std::setw(4) << FileCount << " Files Assembled" << std::endl;
    std::cout << std::setw(4) << FileCount - FilesAssembled << " Files Failed" << std::endl;

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
    std::cout << "Assembling: " << FileName << std::endl;

    for(int Pass = 1; Pass < 3; Pass++)
    {
        AssemblyError::ResetErrorCounters();
        try
        {
            std::cout << "Pass " << Pass << std::endl;

            // Setup Source File stack
            SourceCodeReader Source;
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

                        if (PreProcessorDirective(Line, Directive, Expression)) // Pre-Processor Directive
                        {
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
                            if (Pass == 2)
                               ListingFile.Append();
                        }
                        else // Assembly Source line
                        {
                            try
                            {
                                ExpandDefines(Line, Defines);

                                std::string Label;
                                std::string OpCode;
                                std::vector<std::string>Operands;
                                ExpandTokens(Line, Label, OpCode, Operands);

                                if (Pass == 2)
                                    ListingFile.Append();
                            }
                            catch (AssemblyError Ex)
                            {
                                Error(Source.getFileName(), Source.getLineNumber(), OriginalLine, Ex.Message, Ex.Severity);
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
                    Error(Source.getFileName(), Source.getLineNumber(), OriginalLine, Ex.Message, Ex.Severity);
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
            Error("", 0, "", Ex.Message, Ex.Severity);
            //if(Pass == 2)
            //    ListingFile.AppendError(Ex.Message, Ex.Severity);
        }
    }

    std::cout << std::setw(4) << AssemblyError::WarningCount << " Warnings" << std::endl;
    std::cout << std::setw(4) << AssemblyError::ErrorCount << " Errors" << std::endl;
    std::cout << std::setw(4) << AssemblyError::FatalCount << " Fatal Errors" << std::endl;
    std::cout << std::endl;

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
void Error(const std::string& FileName, const int LineNumber, const std::string& Line, const std::string& Message, const AssemblyErrorSeverity Severity)
{
    switch(Severity)
    {
    case SEVERITY_Warning:  AssemblyError::WarningCount++; break;
    case SEVERITY_Error:    AssemblyError::ErrorCount++;   break;
    case SEVERITY_Fatal:    AssemblyError::FatalCount++;   break;
    }

    std::cout << std::left << std::setw(10) << FileName << std::right << std::setw(5) << LineNumber << ": " << Line << std::endl;
    std::cout << "***************: " << AssemblyError::SeverityName.at(Severity) << ": " << Message << std::endl;
}
