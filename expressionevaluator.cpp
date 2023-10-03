#include "assemblyexception.h"
#include "expressionevaluator.h"

const std::map<std::string, FunctionSpec> ExpressionEvaluator::FunctionTable = {
    { "HIGH",        { FN_HIGH,    1 }},
    { "LOW",         { FN_LOW,     1 }},
    { "ISDEFINED",   { FN_ISDEF,   1 }},
    { "ISDEF",       { FN_ISDEF,   1 }},
    { "ISUNDEFINED", { FN_ISUNDEF, 1 }},
    { "ISUNDEF",     { FN_ISUNDEF, 1 }}
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
    auto Token = TokenStream.Peek();
    if(Token != TOKEN_END && Token != TOKEN_COMMA && Token != TOKEN_CLOSE_BRACE)
        throw AssemblyException("Extra Characters at end of expression", SEVERITY_Error);
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp0
//! Lowest Precedence
//! Logical OR
//! \return
//!
int ExpressionEvaluator::SubExp0()
{
    int Result = SubExp1();
    while(TokenStream.Peek() == TOKEN_LOGICAL_OR)
    {
        TokenStream.Get();
        int rhs = SubExp1();
        Result = ((Result != 0) || (rhs != 0))  ? 1 : 0;
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp0
//! Logical AND
//! \return
//!
int ExpressionEvaluator::SubExp1()
{
    int Result = SubExp2();
    while(TokenStream.Peek() == TOKEN_LOGICAL_AND)
    {
        TokenStream.Get();
        int rhs = SubExp2();
        Result = ((Result != 0) && (rhs != 0)) ? 1 : 0;
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp2
//! Bitwise OR
//! \return
//!
int ExpressionEvaluator::SubExp2()
{
    int Result = SubExp3();
    while(TokenStream.Peek() == TOKEN_BITWISE_OR)
    {
        TokenStream.Get();
        Result |= SubExp3();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp3
//! Bitwise XOR
//! \return
//!
int ExpressionEvaluator::SubExp3()
{
    int Result = SubExp4();
    while(TokenStream.Peek() == TOKEN_BITWISE_XOR)
    {
        TokenStream.Get();
        Result ^= SubExp4();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp4
//! Bitwise AND
//! \return
//!
int ExpressionEvaluator::SubExp4()
{
    int Result = SubExp5();
    while(TokenStream.Peek() == TOKEN_BITWISE_AND)
    {
        TokenStream.Get();
        Result &= SubExp5();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp5
//! EQUAL / NOT EQUAL
//! \return
//!
int ExpressionEvaluator::SubExp5()
{
    int Result = SubExp6();
    while(TokenStream.Peek() == TOKEN_EQUAL || TokenStream.Peek() == TOKEN_NOT_EQUAL)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_EQUAL:
            Result = (Result == SubExp6()) ? 1 : 0;
            break;
        case TOKEN_NOT_EQUAL:
            Result = (Result == SubExp6()) ? 0 : 1;
            break;
        default:
            break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp6
//! LESS / LESS or EQUAL / GREATER / GREATER or EQUAL
//! \return
//!
int ExpressionEvaluator::SubExp6()
{
    int Result = SubExp7();
    while(TokenStream.Peek() == TOKEN_LESS || TokenStream.Peek() == TOKEN_LESS_OR_EQUAL || TokenStream.Peek() == TOKEN_GREATER || TokenStream.Peek() == TOKEN_GREATER_OR_EQUAL)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_LESS:
            Result = (Result < SubExp7()) ? 1 : 0;
            break;
        case TOKEN_LESS_OR_EQUAL:
            Result = (Result <= SubExp7()) ? 1 : 0;
            break;
        case TOKEN_GREATER:
            Result = (Result > SubExp7()) ? 1 : 0;
            break;
        case TOKEN_GREATER_OR_EQUAL:
            Result = (Result >= SubExp7()) ? 1 : 0;
            break;
        default:
            break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp7
//! Shift LEFT / RIGHT
//! \return
//!
int ExpressionEvaluator::SubExp7()
{
    int Result = SubExp8();
    while(TokenStream.Peek() == TOKEN_SHIFT_LEFT || TokenStream.Peek() == TOKEN_SHIFT_RIGHT)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_SHIFT_LEFT:
            Result <<= SubExp8();
            break;
        case TOKEN_SHIFT_RIGHT:
            Result >>= SubExp8();
            break;
        default:
            break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp8
//! ADDITION / SUPTRACTION
//! \return
//!
int ExpressionEvaluator::SubExp8()
{
    int Result = SubExp9();
    while(TokenStream.Peek() == TOKEN_PLUS || TokenStream.Peek() == TOKEN_MINUS)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_PLUS:
            Result += SubExp9();
            break;
        case TOKEN_MINUS:
            Result -= SubExp9();
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
int ExpressionEvaluator::SubExp9()
{
    int Result = SubExp10();
    while(TokenStream.Peek() == TOKEN_MULTIPLY || TokenStream.Peek() == TOKEN_DIVIDE || TokenStream.Peek() == TOKEN_REMAINDER)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
        case TOKEN_MULTIPLY:
            Result *= SubExp10();
            break;
        case TOKEN_DIVIDE:
        {
            int Operand = SubExp10();
            if(Operand == 0)
                throw AssemblyException("Divide by zero", SEVERITY_Error);
            Result /= Operand;
            break;
        }
        case TOKEN_REMAINDER:
        {
            int Operand = SubExp10();
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
//! \brief ExpressionEvaluator::SubExp8
//! Unary PLUS / MINUS / Bitwise NOT / Logical NOT
//! \return
//!
int ExpressionEvaluator::SubExp10()
{
    int Result;
    auto Token = TokenStream.Peek();
    if(Token == TOKEN_PLUS || Token == TOKEN_MINUS || Token == TOKEN_BITWISE_NOT || Token == TOKEN_LOGICAL_NOT)
    {
        TokenStream.Get();
        switch(Token)
        {
        case TOKEN_PLUS:
            return SubExp10();
            break;
        case TOKEN_MINUS:
            return -SubExp10();
            break;
        case TOKEN_BITWISE_NOT:
            return ~SubExp10();
            break;
        case TOKEN_LOGICAL_NOT:
            return SubExp10() ? 1 : 0;
            break;
        default:
            break;
        }
    }
    return SubExp11();
}

//!
//! \brief ExpressionEvaluator::SubExp9
//! Highest Precedence
//! Constant / Label / Function Call / Bracketed Expression
//! \return
//!
int ExpressionEvaluator::SubExp11()
{
    int Result = 0;
    auto Token = TokenStream.Get();
    switch(Token)
    {
    case TOKEN_NUMBER:
        Result = TokenStream.IntegerValue;
        break;

    case TOKEN_DOT:
        Result = ProgramCounter;
        break;

    case TOKEN_OPEN_BRACE: // Bracketed Expression
        Result = SubExp0();
        if (TokenStream.Peek() != TOKEN_CLOSE_BRACE)
            throw AssemblyException("Expected ')'", SEVERITY_Error);
        else
            TokenStream.Get();
        break;

    case TOKEN_LABEL:
    {
        std::string Label = TokenStream.StringValue;
        if(TokenStream.Peek() == TOKEN_OPEN_BRACE)
        {
            TokenStream.Get();

            auto FunctionSpec = FunctionTable.find(Label);
            if(FunctionSpec == FunctionTable.end())
                throw AssemblyException("Unknown function call", SEVERITY_Error);

            std::vector<int> Arguments = { };
            switch(FunctionSpec->second.ID)
            {
            case FN_LOW:
                if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                    throw AssemblyException("Incorrect number of arguments: LOW expects 1 argument", SEVERITY_Error);
                Result = Arguments[0] & 0xFF;
                break;
            case FN_HIGH:
                if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                    throw AssemblyException("Incorrect number of arguments: HIGH expects 1 argument", SEVERITY_Error);
                Result = (Arguments[0] >> 8) & 0xFF;
                break;
            case FN_ISDEF:
                if(TokenStream.Peek() == TOKEN_LABEL)
                {
                    Result = 0;
                    TokenStream.Get();
                    std::string Label = TokenStream.StringValue;
                    if(TokenStream.Peek() == TOKEN_CLOSE_BRACE)
                    {
                        TokenStream.Get();
                        if (LocalSymbols && Local->Symbols.find(Label) != Local->Symbols.end())
                            Result = 1;
                        if (Global->Symbols.find(Label) != Global->Symbols.end())
                            Result = 1;
                    }
                    else
                        throw AssemblyException("')' Expected", SEVERITY_Error);
                }
                else
                    throw AssemblyException("ISDEF expects a single LABEL argument", SEVERITY_Error);
                break;
            case FN_ISUNDEF:
                if(TokenStream.Peek() == TOKEN_LABEL)
                {
                    Result = 1;
                    TokenStream.Get();
                    std::string Label = TokenStream.StringValue;
                    if(TokenStream.Peek() == TOKEN_CLOSE_BRACE)
                    {
                        TokenStream.Get();
                        if (LocalSymbols && Local->Symbols.find(Label) != Local->Symbols.end())
                            Result = 0;
                        if (Global->Symbols.find(Label) != Global->Symbols.end())
                            Result = 0;
                    }
                    else
                        throw AssemblyException("')' Expected", SEVERITY_Error);
                }
                else
                    throw AssemblyException("ISNDEF expects a single LABEL argument", SEVERITY_Error);
                break;
            }
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
//! \brief ExpressionEvaluator::GetFunctionArguments
//! Evaluate function arguments and populate the passed vector.
//! \param Arguments
//! \param Count
//! \return True if correct number of arguments were found
//!
bool ExpressionEvaluator::GetFunctionArguments(std::vector<int> &Arguments, int Count)
{
    if(TokenStream.Peek() == TOKEN_CLOSE_BRACE)
        TokenStream.Get();
    else
    {
        Arguments.push_back(SubExp0());
        while(TokenStream.Peek() == TOKEN_COMMA)
        {
            TokenStream.Get();
            Arguments.push_back(SubExp0());
        }
        if(TokenStream.Peek() == TOKEN_CLOSE_BRACE)
            TokenStream.Get();
        else
            throw AssemblyException("Syntax error in argument list", SEVERITY_Error);
    }
    return Arguments.size() == Count;
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
            if(Symbol->second.Value.has_value())
                return Symbol->second.Value.value();
            else
                throw AssemblyException(fmt::format("Label '{Label}' is not yet assigned", fmt::arg("Label", Label)), SEVERITY_Error);
        }
    }

    auto Symbol = Global->Symbols.find(Label);
    if(Symbol != Global->Symbols.end())
    {
        if(Symbol->second.Value.has_value())
            return Symbol->second.Value.value();
        else
            throw AssemblyException(fmt::format("Label '{Label}' is not yet assigned", fmt::arg("Label", Label)), SEVERITY_Error);
    }
    throw AssemblyException(fmt::format("Label '{Label}' not found", fmt::arg("Label", Label)), SEVERITY_Error);
}
