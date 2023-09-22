#include "expressionevaluator.h"

ExpressionEvaluator::ExpressionEvaluator(const blob& Global) : Global(&Global)
{
    LocalSymbols = false;
}

void ExpressionEvaluator::AddLocalSymbols(const blob* Local)
{
    this->Local = Local;
    LocalSymbols = true;
}

uint16_t ExpressionEvaluator::Evaluate(std::string& Expression)
{
    std::string test = "FINDRAM"; // TODO Evaluate the expression
    return SymbolValue(test);
}

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
