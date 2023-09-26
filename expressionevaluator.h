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
    int Evaluate(std::string& Expression);

private:
    const SymbolTable* Local;
    const SymbolTable* Global;
    bool LocalSymbols;      // Denotess if a local blob is available for symbol lookups

    uint16_t SymbolValue(std::string& Label);

    ExpressionTokenizer TokenStream;

    int SubExp0();
    int SubExp1();
    int SubExp2();
    int SubExp3();
    int SubExp4();
    int SubExp5();
    int SubExp6();
    int SubExp7();
};

#endif // EXPRESSIONEVALUATOR_H
