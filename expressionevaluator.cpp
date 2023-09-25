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

uint16_t ExpressionEvaluator::Evaluate(std::string& Expression)
{
    TokenStream.Initialize(Expression);

    TokenEnum T;
    while((T = TokenStream.GetToken()) != TOKEN_END)
    {
        auto S = TokenStream.StringValue;
        auto v = TokenStream.IntegerValue;
    }


    return 1;
}

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
