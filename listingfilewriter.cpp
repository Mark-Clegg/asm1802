#include <filesystem>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <iomanip>
#include <vector>
#include "listingfilewriter.h"

ListingFileWriter::ListingFileWriter(const std::string& FileName, ErrorTable& Errors, bool Enabled) :
    Errors { Errors },
    Enabled { Enabled }
{
    File = std::filesystem::path(FileName);
    File.replace_extension("lst");
    if(std::filesystem::exists(File))
        std::filesystem::remove(File);
    ListFileName = File;
}

ListingFileWriter::~ListingFileWriter()
{
    if(ListStream.is_open())
        ListStream.close();
}

void ListingFileWriter::Reset()
{
    if(ListStream.is_open())
        ListStream.close();
    if(std::filesystem::exists(File))
        std::filesystem::remove(File);
}

void ListingFileWriter::Append(const std::string& FullFileName, int LineNumber, const std::string& MacroName, int MacroLineNumber, const std::string& Line, const bool InMacro)
{
    std::string FileName = std::filesystem::path(FullFileName).filename();
    std::string FileRef;
    if(InMacro)
        FileRef = FileName + "::" + MacroName;
    else
        FileRef = FileName;

    if(Enabled)
    {
        if(!ListStream.is_open())
        {
            ListStream.open(ListFileName, std::ofstream::out | std::ofstream::trunc);
        }
        if(InMacro)
            fmt::println(ListStream, "[{filename:22.22} {linenumber:05}:{macrolinenumber:02}]                       {line}",
                         fmt::arg("filename", FileRef),
                         fmt::arg("linenumber", LineNumber - 1),
                         fmt::arg("macrolinenumber", MacroLineNumber),
                         fmt::arg("line", Line)
                        );
        else
            fmt::println(ListStream, "[{filename:22.22} {linenumber:05}   ]                       {line}",
                         fmt::arg("filename", FileName),
                         fmt::arg("linenumber", LineNumber),
                         fmt::arg("line", Line)
                        );

        PrintError(FileName, LineNumber, MacroName, MacroLineNumber, InMacro);
    }
}

void ListingFileWriter::Append(const std::string& FullFileName, int LineNumber, const std::string& MacroName, int MacroLineNumber, const std::string& Line, const bool InMacro, const std::uint16_t Address, const std::vector<std::uint8_t>& Data)
{
    std::string FileName = std::filesystem::path(FullFileName).filename();
    std::string FileRef;
    if(InMacro)
        FileRef = FileName+"::"+MacroName;
    else
        FileRef = FileName;

    if(Enabled)
    {
        if(!ListStream.is_open())
        {
            ListStream.open(ListFileName, std::ofstream::out | std::ofstream::trunc);
        }
        if(Data.size() == 0)
        {
            if(InMacro)
                fmt::println(ListStream, "[{filename:22.22} {linenumber:05}:{macrolinenumber:02}]  {address:04X}                 {line}",
                             fmt::arg("filename", FileRef),
                             fmt::arg("linenumber", LineNumber - 1),
                             fmt::arg("macrolinenumber", MacroLineNumber),
                             fmt::arg("address", Address),
                             fmt::arg("line", Line)
                            );
            else
                fmt::println(ListStream, "[{filename:22.22} {linenumber:05}   ]  {address:04X}                 {line}",
                             fmt::arg("filename", FileName),
                             fmt::arg("linenumber", LineNumber),
                             fmt::arg("address", Address),
                             fmt::arg("line", Line)
                            );
        }
        else
        {
            int LineCount = (Data.size() - 1) / 4 + 1;
            for(int i = 0; i < std::min(LineCount, 16); i++)
            {
                if(i == 0)
                    if(InMacro)
                        fmt::print(ListStream, "[{filename:22.22} {linenumber:05}:{macrolinenumber:02}]  {address:04X}   ",
                                   fmt::arg("filename", FileRef),
                                   fmt::arg("linenumber", LineNumber - 1),
                                   fmt::arg("macrolinenumber", MacroLineNumber),
                                   fmt::arg("address", Address)
                                  );
                    else
                        fmt::print(ListStream, "[{filename:22.22} {linenumber:05}   ]  {address:04X}   ",
                                   fmt::arg("filename", FileName),
                                   fmt::arg("linenumber", LineNumber),
                                   fmt::arg("address", Address)
                                  );
                else
                    fmt::print(ListStream, "{space:42}", fmt::arg("space", " "));

                for(int j = 0; j < 4; j++)
                    if((i*4)+j < Data.size())
                        fmt::print(ListStream, "{byte:02X} ", fmt::arg("byte", Data[i*4+j]));
                    else
                        fmt::print(ListStream, "{space:2} ", fmt::arg("space", ""));
                if(i == 0)
                    fmt::print(ListStream, "  {line}", // Initial spaces to pad line start to an 8 character boundary (to align tabs)
                               fmt::arg("line", Line)
                              );
                fmt::println(ListStream, "");
            }
            if(LineCount > 16)
            {
                fmt::println(ListStream, "{space:42}.. .. .. ..           (remaining {bytes} bytes omitted from listing)",
                             fmt::arg("space", " "),
                             fmt::arg("bytes", Data.size()-64));
            }
        }
        PrintError(FileName, LineNumber, MacroName, MacroLineNumber, InMacro);
    }
}

void ListingFileWriter::PrintError(const std::string& FileName, const int LineNumber, const std::string& MacroName, const int MacroLineNumber, const bool InMacro)
{
    std::string FileRef;
    std::pair<int,int> LineRef;
    if(InMacro)
    {
        FileRef = FileName+"::"+MacroName;
        LineRef = { LineNumber, MacroLineNumber };
    }
    else
    {
        FileRef = FileName;
        LineRef = { LineNumber, 0 };
    }

    if(Errors.count(FileRef) != 0)
    {
        auto range = Errors[FileRef].equal_range(LineRef);
        for(auto it = range.first; it != range.second; it++)
        {
            auto MsgSevPair = it->second;
            std::string Message = MsgSevPair.first;
            AssemblyErrorSeverity Severity = MsgSevPair.second;
            fmt::println(ListStream, "**********************************************{severity:*>15}:  {message}",
                         fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                         fmt::arg("message", Message));
        }
    }
}

void ListingFileWriter::AppendGlobalErrors()
{
    if(Enabled)
    {
        if(Errors.count("") != 0)
        {
            auto range = Errors[""].equal_range({0,0});
            for(auto& it = range.first; it != range.second; it++)
            {
                auto MsgSevPair = it->second;
                std::string Message = MsgSevPair.first;
                AssemblyErrorSeverity Severity = MsgSevPair.second;
                fmt::println(ListStream, "**************************************{severity:*>15}:  {message}",
                             fmt::arg("severity", " "+AssemblyException::SeverityName.at(Severity)),
                             fmt::arg("message", Message));
            }
        }
    }
}

void ListingFileWriter::AppendSymbols(const std::string& Name, const SymbolTable& Blob)
{
    if(Enabled)
    {
        fmt::println(ListStream, "");

        if(Blob.Name.empty())
            fmt::println(ListStream, "{Name:-^116}", fmt::arg("Name", "Global Symbols"));
        else
        {
            std::string NameAndSize = fmt::format("{Name} @ ${Address:04X} ({Size} (${Size:04X}) bytes)",
                                                  fmt::arg("Name", Name),
                                                  fmt::arg("Size",  Blob.CodeSize),
                                                  fmt::arg("Address", Blob.Symbols.at(Name).Value.value()));
            fmt::println(ListStream, "{Name:-^116}", fmt::arg("Name", NameAndSize));
        }

        int c = 0;
        for(auto& Symbol : Blob.Symbols)
            if(!Symbol.second.HideFromSymbolTable)
            {
                fmt::print(ListStream, "{Name:15} ", fmt::arg("Name", Symbol.first));
                if(Symbol.second.Value.has_value())
                {
                    if(Symbol.second.Value.value() >= -65536 && Symbol.second.Value.value() <= 65535)
                        fmt::print(ListStream, "{Address:04X}", fmt::arg("Address", Symbol.second.Value.value() & 0xFFFF));
                    else
                    {
                        // Fixup for values over 2 bytes long
                        if(c == 3)
                        {
                            fmt::println(ListStream, "");
                            c++;
                        }
                        fmt::print(ListStream, "{Address:08X}", fmt::arg("Address", (unsigned long)Symbol.second.Value.value()));
                        c++;
                        if(c % 5 != 0)
                            fmt::print(ListStream, "            ");
                    }
                }
                else
                    fmt::print(ListStream, "----");
                if(++c % 5 == 0)
                    fmt::println(ListStream, "");
                else
                    fmt::print(ListStream, "    ");
            }
        fmt::println(ListStream, "");
    }
}
