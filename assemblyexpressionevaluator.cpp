#include "assemblyexpressionevaluator.h"
#include "expressionexception.h"
#include "utils.h"

const std::map<std::string, AssemblyExpressionEvaluator::FunctionSpec> AssemblyExpressionEvaluator::FunctionTable =
{
    { "CPU",         { FN_PROCESSOR, 1 }},
    { "PROCESSOR",   { FN_PROCESSOR, 1 }},
    { "HIGH",        { FN_HIGH,      1 }},
    { "LOW",         { FN_LOW,       1 }},
    { "ISDEF",       { FN_ISDEF,     1 }},
    { "ISNDEF",      { FN_ISNDEF,    1 }}
};

AssemblyExpressionEvaluator::AssemblyExpressionEvaluator(const SymbolTable& Global, uint16_t ProgramCounter, CPUTypeEnum Processor) :
    ExpressionEvaluatorBase(),
    Global(&Global),
    ProgramCounter(ProgramCounter),
    Processor(Processor)
{
    LocalSymbols = false;
}

//!
//! \brief ExpressionEvaluator::AddLocalSymbols
//! Add local symbols table to scope for lable lookups
//! \param Local
//!
void AssemblyExpressionEvaluator::AddLocalSymbols(const SymbolTable* Local)
{
    this->Local = Local;
    LocalSymbols = true;
}

int AssemblyExpressionEvaluator::AtomValue()
{
    int Result = 0;
    auto Token = TokenStream.Get();
    switch(Token)
    {
        case ExpressionTokenizer::TOKEN_QUOTED_STRING:
            throw ExpressionException("Unexpected string literal");
            break;
        case ExpressionTokenizer::TOKEN_NUMBER:
            Result = TokenStream.IntegerValue;
            break;

        case ExpressionTokenizer::TOKEN_DOLLAR:
        case ExpressionTokenizer::TOKEN_DOT:
            Result = ProgramCounter;
            break;

        case ExpressionTokenizer::TOKEN_OPEN_BRACE: // Bracketed Expression
            Result = EvaluateSubExpression();
            if (TokenStream.Peek() != ExpressionTokenizer::TOKEN_CLOSE_BRACE)
                throw ExpressionException("Expected ')'");
            else
                TokenStream.Get();
            break;

        case ExpressionTokenizer::TOKEN_LABEL:
        {
            std::string Label = TokenStream.StringValue;
            if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_OPEN_BRACE)
            {
                TokenStream.Get();

                auto FunctionSpec = FunctionTable.find(Label);
                if(FunctionSpec == FunctionTable.end())
                    throw ExpressionException("Unknown function call");

                std::vector<int> Arguments = { };
                switch(FunctionSpec->second.ID)
                {
                    case FN_PROCESSOR:
                    {
                        std::string Value;
                        if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_QUOTED_STRING)
                            TokenStream.Get();
                        else if(!TokenStream.GetCustomToken(std::regex(R"(^(([Cc][Dd][Pp])?180[2456][Aa]?).*)")))
                            throw ExpressionException("Expected Processor designation");

                        Value = TokenStream.StringValue;
                        ToUpper(Value);

                        if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_CLOSE_BRACE)
                        {
                            TokenStream.Get();

                            auto CPU = OpCodeTable::CPUTable.find(Value);
                            if(CPU == OpCodeTable::CPUTable.end())
                                throw ExpressionException("Unrecognised processor designation");
                            Result = CPU->second <= Processor ? 1 : 0;
                        }
                        else
                            throw ExpressionException("Extra characters after Processor designation");
                        break;
                    }
                    case FN_LOW:
                        if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                            throw ExpressionException("Incorrect number of arguments: LOW expects 1 argument");
                        Result = Arguments[0] & 0xFF;
                        break;
                    case FN_HIGH:
                        if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                            throw ExpressionException("Incorrect number of arguments: HIGH expects 1 argument");
                        Result = (Arguments[0] >> 8) & 0xFF;
                        break;
                    case FN_ISDEF:
                        if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_LABEL)
                        {
                            Result = 0;
                            TokenStream.Get();
                            std::string Label = TokenStream.StringValue;
                            if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_CLOSE_BRACE)
                            {
                                TokenStream.Get();
                                if (LocalSymbols && Local->Symbols.find(Label) != Local->Symbols.end())
                                    Result = 1;
                                if (Global->Symbols.find(Label) != Global->Symbols.end())
                                    Result = 1;
                            }
                            else
                                throw ExpressionException("')' Expected");
                        }
                        else
                            throw ExpressionException("ISDEF expects a single LABEL argument");
                        break;
                    case FN_ISNDEF:
                        if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_LABEL)
                        {
                            Result = 1;
                            TokenStream.Get();
                            std::string Label = TokenStream.StringValue;
                            if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_CLOSE_BRACE)
                            {
                                TokenStream.Get();
                                if (LocalSymbols && Local->Symbols.find(Label) != Local->Symbols.end())
                                    Result = 0;
                                if (Global->Symbols.find(Label) != Global->Symbols.end())
                                    Result = 0;
                            }
                            else
                                throw ExpressionException("')' Expected");
                        }
                        else
                            throw ExpressionException("ISNDEF expects a single LABEL argument");
                        break;
                }
            }
            else
                Result = SymbolValue(Label);
            break;
        }
        default: // Should never happen
            throw ExpressionException("Current Token Not Yet Implemented");
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SymbolValue
//! Lookup the given Label in the local and global symbol tables
//! Local Table takes precedence.
//! \param Label
//! \return
//!
uint16_t AssemblyExpressionEvaluator::SymbolValue(std::string Label)
{
    if(LocalSymbols)
    {
        auto Symbol = Local->Symbols.find(Label);
        if(Symbol != Local->Symbols.end())
        {
            if(Symbol->second.Value.has_value())
                return Symbol->second.Value.value();
            else
                throw ExpressionException(fmt::format("Label '{Label}' is not yet assigned", fmt::arg("Label", Label)));
        }
    }

    auto Symbol = Global->Symbols.find(Label);
    if(Symbol != Global->Symbols.end())
    {
        if(Symbol->second.Value.has_value())
            return Symbol->second.Value.value();
        else
            throw ExpressionException(fmt::format("Label '{Label}' is not yet assigned", fmt::arg("Label", Label)));
    }
    throw ExpressionException(fmt::format("Label '{Label}' not found", fmt::arg("Label", Label)));
}
