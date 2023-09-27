#include "assemblyexception.h"
#include "expressionevaluator.h"

const std::map<std::string, FunctionSpec> ExpressionEvaluator::FunctionTable = {
    { "HIGH", { FN_HIGH, 1 }},
    { "LOW",  { FN_LOW,  1 }}
};

ExpressionEvaluator::ExpressionEvaluator(const SymbolTable& Global, uint16_t ProgramCounter) : Global(&Global), ProgramCounter(ProgramCounter)
{
    LocalSymbols = false;
}

//!
//! \brief ExpressionEvaluator::AddLocalSymbols
//! Add local symbols table to scope for lable lookups
//! \param Local
//!
void ExpressionEvaluator::AddLocalSymbols(const SymbolTable* Local)
{
    this->Local = Local;
    LocalSymbols = true;
}

//!
//! \brief ExpressionEvaluator::Evaluate
//! Evaluate the given expression
//! \param Expression
//! \return
//!
int ExpressionEvaluator::Evaluate(std::string& Expression)
{
    TokenStream.Initialize(Expression);

    int Result = SubExp0();
    if(TokenStream.Peek() != TOKEN_END)
        throw AssemblyException("Extra Characters at end of expression", SEVERITY_Error);
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp0
//! Lowest Precedence
//! Bitwise OR
//! \return
//!
int ExpressionEvaluator::SubExp0()
{
    int Result = SubExp1();
    while(TokenStream.Peek() == TOKEN_OR)
    {
        TokenStream.Get();
        Result |= SubExp1();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp1
//! Bitwise XOR
//! \return
//!
int ExpressionEvaluator::SubExp1()
{
    int Result = SubExp2();
    while(TokenStream.Peek() == TOKEN_XOR)
    {
        TokenStream.Get();
        Result ^= SubExp2();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp2
//! Bitwise AND
//! \return
//!
int ExpressionEvaluator::SubExp2()
{
    int Result = SubExp3();
    while(TokenStream.Peek() == TOKEN_AND)
    {
        TokenStream.Get();
        Result &= SubExp3();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp3
//! Shift LEFT / RIGHT
//! \return
//!
int ExpressionEvaluator::SubExp3()
{
    int Result = SubExp4();
    while(TokenStream.Peek() == TOKEN_SHIFTLEFT || TokenStream.Peek() == TOKEN_SHIFTRIGHT)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_SHIFTLEFT:
            Result <<= SubExp4();
            break;
        case TOKEN_SHIFTRIGHT:
            Result >>= SubExp4();
            break;
        default:
            break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp4
//! ADDITION / SUPTRACTION
//! \return
//!
int ExpressionEvaluator::SubExp4()
{
    int Result = SubExp5();
    while(TokenStream.Peek() == TOKEN_PLUS || TokenStream.Peek() == TOKEN_MINUS)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_PLUS:
            Result += SubExp5();
            break;
        case TOKEN_MINUS:
            Result -= SubExp5();
            break;
        default:
            break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp5
//! MULTIPLY / DIVIDE / REMAINDER
//! \return
//!
int ExpressionEvaluator::SubExp5()
{
    int Result = SubExp6();
    while(TokenStream.Peek() == TOKEN_MULTIPLY || TokenStream.Peek() == TOKEN_DIVIDE || TokenStream.Peek() == TOKEN_REMAINDER)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_MULTIPLY:
            Result *= SubExp6();
            break;
        case TOKEN_DIVIDE:
        {
            int Operand = SubExp6();
            if(Operand == 0)
                throw AssemblyException("Divide by zero", SEVERITY_Error);
            Result /= Operand;
            break;
        }
        case TOKEN_REMAINDER:
        {
            int Operand = SubExp6();
            if(Operand == 0)
                throw AssemblyException("Divide by zero", SEVERITY_Error);
            Result %= Operand;
            break;
        }
        default:
            break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp6
//! Unary PLUS / MINUS / NOT
//! \return
//!
int ExpressionEvaluator::SubExp6()
{
    int Result;
    auto Token = TokenStream.Peek();
    if(Token == TOKEN_PLUS || Token == TOKEN_MINUS || Token == TOKEN_NOT)
        switch(Token)
        {
        case TOKEN_PLUS:
            TokenStream.Get();
            return SubExp7();
            break;
        case TOKEN_MINUS:
            TokenStream.Get();
            return -SubExp7();
            break;
        case TOKEN_NOT:
            TokenStream.Get();
            return ~SubExp7();
            break;
        default:
            break;
        }
    return SubExp7();
}

//!
//! \brief ExpressionEvaluator::SubExp7
//! Highest Precedence
//! Constant / Label / Function Call / Bracketed Expression
//! \return
//!
int ExpressionEvaluator::SubExp7()
{
    int Result;
    auto Token = TokenStream.Get();
    switch(Token)
    {
    case TOKEN_NUMBER:
        Result = TokenStream.IntegerValue;
        break;

    case TOKEN_DOT:
        Result = ProgramCounter;
        break;

    case TOKEN_OPENBRACE: // Bracketed Expression
        Result = SubExp0();
        if (TokenStream.Peek() != TOKEN_CLOSEBRACE)
            throw AssemblyException("Expected ')'", SEVERITY_Error);
        else
            TokenStream.Get();
        break;

    case TOKEN_LABEL:
    {
        std::string Label = TokenStream.StringValue;
        if(TokenStream.Peek() == TOKEN_OPENBRACE)
        {
            TokenStream.Get();
            std::vector<int> Arguments = { };
            if(TokenStream.Peek() != TOKEN_CLOSEBRACE)
            {
                Arguments.push_back(SubExp0());
            }
            while(TokenStream.Peek() == TOKEN_COMMA)
            {
                TokenStream.Get();
                Arguments.push_back(SubExp0());
            }
            if(TokenStream.Peek() == TOKEN_CLOSEBRACE)
                TokenStream.Get();
            else
                throw AssemblyException("Syntax error in argument list");

            auto FunctionSpec = FunctionTable.find(Label);
            if(FunctionSpec == FunctionTable.end())
                throw AssemblyException("Unknown function call", SEVERITY_Error);
            if(FunctionSpec->second.Arguments != Arguments.size())
                throw AssemblyException(fmt::format("Incorrect number of arguments: {Arg} expects {Count} argument(s)", fmt::arg("Arg", Label), fmt::arg("Count", FunctionSpec->second.Arguments)), SEVERITY_Error);
            switch(FunctionSpec->second.ID)
            {
            case FN_LOW:
                return Arguments[0] & 0xFF;
                break;
            case FN_HIGH:
                return (Arguments[0] >> 8) & 0xFF;
                break;
            }

            //Result = call_function_with(Arguments); // TODO Function Evaluation
            Result = 0;
        }
        else
            Result = SymbolValue(Label);
        break;
    }
    default: // Should never happen
        throw AssemblyException("Current Token Not Yet Implemented", SEVERITY_Error);
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
uint16_t ExpressionEvaluator::SymbolValue(std::string& Label)
{
    if(LocalSymbols)
    {
        auto Symbol = Local->Symbols.find(Label);
        if(Symbol != Local->Symbols.end())
        {
            if(Symbol->second.has_value())
                return Symbol->second.value();
            else
                throw AssemblyException(fmt::format("Label '{Label}' is not yet assigned", fmt::arg("Label", Label)), SEVERITY_Error);
        }
    }

    auto Symbol = Global->Symbols.find(Label);
    if(Symbol != Global->Symbols.end())
    {
        if(Symbol->second.has_value())
            return Symbol->second.value();
        else
            throw AssemblyException(fmt::format("Label '{Label}' is not yet assigned", fmt::arg("Label", Label)), SEVERITY_Error);
    }
    throw AssemblyException(fmt::format("Label '{Label}' not found", fmt::arg("Label", Label)), SEVERITY_Error);
}
