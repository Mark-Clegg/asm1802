#ifndef EXPRESSIONEVALUATOR_H
#define EXPRESSIONEVALUATOR_H

#include <fmt/core.h>
#include <string>
#include "assemblyexception.h"
#include "blob.h"

class ExpressionEvaluator
{
public:
    ExpressionEvaluator(const blob& Global);
    void AddLocalSymbols(const blob* Local);
    uint16_t Evaluate(std::string& Expression);

private:
    const blob* Local;
    const blob* Global;
    bool LocalSymbols;      // Denotess if a local blob is available for symbol lookups

    uint16_t SymbolValue(std::string& Label);
};

#endif // EXPRESSIONEVALUATOR_H
