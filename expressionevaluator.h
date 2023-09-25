#ifndef EXPRESSIONEVALUATOR_H
#define EXPRESSIONEVALUATOR_H

#include <fmt/core.h>
#include <string>
#include "expressiontokenizer.h"
#include "symboltable.h"

class ExpressionEvaluator
{
public:
    ExpressionEvaluator(const SymbolTable& Global);
    void AddLocalSymbols(const SymbolTable* Local);
    uint16_t Evaluate(std::string& Expression);

private:
    const SymbolTable* Local;
    const SymbolTable* Global;
    bool LocalSymbols;      // Denotess if a local blob is available for symbol lookups

    uint16_t SymbolValue(std::string& Label);

    ExpressionTokenizer TokenStream;
};

#endif // EXPRESSIONEVALUATOR_H
