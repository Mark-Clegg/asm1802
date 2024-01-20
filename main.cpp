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
#include "assembler.h"
#include "assemblyexception.h"
#include "preprocessor.h"
#include "utils.h"

namespace fs = std::filesystem;

std::string Version("1.0");

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

    CPUTypeEnum InitialProcessor = CPUTypeEnum::CPU_1802;
    bool Listing = false;
    PreProcessor AssemblerPreProcessor;
    bool KeepPreprocessor = false;
    bool Symbols = false;
    Assembler::OutputFormatEnum OutputFormat = Assembler::OutputFormatEnum::NONE;
    bool NoRegisters = false;   // Suppress pre-defined Register equates
    bool NoPorts     = false;   // Suppress pre-defined Port equates

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
                if(Assembler::OutputFormatLookup.find(Mode) == Assembler::OutputFormatLookup.end())
                    fmt::println("** Unrecognised binary output mode. Defaulting to NONE");
                else
                    OutputFormat = Assembler::OutputFormatLookup.at(Mode);
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
                fmt::println("\tCreate output file in \"intelhex\", \"idiiot4\" format, or \"none\"");
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

                Result = Assembler(PreProcessedInputFile, InitialProcessor, Listing, Symbols, NoRegisters, NoPorts, OutputFormat).Run();
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
