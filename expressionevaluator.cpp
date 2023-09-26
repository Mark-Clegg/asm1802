#include "assemblyexception.h"
#include "expressionevaluator.h"

ExpressionEvaluator::ExpressionEvaluator(const SymbolTable& Global) : Global(&Global)
{
    LocalSymbols = false;
}

void ExpressionEvaluator::AddLocalSymbols(const SymbolTable* Local)
{
    this->Local = Local;
    LocalSymbols = true;
}

int ExpressionEvaluator::Evaluate(std::string& Expression)
{
    TokenStream.Initialize(Expression);

    int Result = SubExp0();
    if(TokenStream.Peek() != TOKEN_END)
        throw AssemblyException("Extra Characters at end of expression", SEVERITY_Error);
    return Result;
}

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
        }
    }
    return Result;
}

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
            Result -+ SubExp5();
            break;
        }
    }
    return Result;
}

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
            Result /= SubExp6();
            break;
        case TOKEN_REMAINDER:
            Result %= SubExp6();
            break;
        }
    }
    return Result;
}

int ExpressionEvaluator::SubExp6()
{
    int Result;
    auto Token = TokenStream.Peek();
    if(Token == TOKEN_PLUS || Token == TOKEN_MINUS)
        switch(Token)
        {
        case TOKEN_PLUS:
            return SubExp7();
            break;
        case TOKEN_MINUS:
            return -SubExp7();
            break;
        }
    return SubExp7();
}

int ExpressionEvaluator::SubExp7()
{
    int Result;
    auto Token = TokenStream.Get();
    switch(Token)
    {
    case TOKEN_NUMBER:
        Result = TokenStream.IntegerValue;
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
            int Argument = SubExp0();
            if (TokenStream.Peek() != TOKEN_CLOSEBRACE)
                throw AssemblyException("Expected ')'", SEVERITY_Error);
            else
                TokenStream.Get();
            //Result = call_function_with(Argument);
            Result = 0;
        }
        else
            Result = SymbolValue(Label);
        break;
    }
    default:
        throw AssemblyException("Current Token Not Yet Implemented", SEVERITY_Error);
    }
    return Result;
}

    //TokenEnum T;
    //while((T = TokenStream.GetToken()) != TOKEN_END)
    //{
    //    auto S = TokenStream.StringValue;
    //    auto v = TokenStream.IntegerValue;
    //}

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
