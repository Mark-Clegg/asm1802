#ifndef EXPRESSIONEVALUATOR_H
#define EXPRESSIONEVALUATOR_H

#include <fmt/core.h>
#include <string>
#include "expressiontokenizer.h"
#include "symboltable.h"

enum FunctionEnum
{
    FN_HIGH,
    FN_LOW,
    FN_ISDEF,
    FN_ISUNDEF
};

struct FunctionSpec
{
    FunctionEnum ID;
    int Arguments;
};

class ExpressionEvaluator
{
public:
    ExpressionEvaluator(const SymbolTable& Global, uint16_t ProgramCounter);
    void AddLocalSymbols(const SymbolTable* Local);
    int Evaluate(std::string& Expression);

private:
    static const std::map<std::string, FunctionSpec> FunctionTable;
    const SymbolTable* Local;
    const SymbolTable* Global;
    bool LocalSymbols;      // Denotess if a local blob is available for symbol lookups

    const uint16_t ProgramCounter;
    uint16_t SymbolValue(std::string& Label);
    bool GetFunctionArguments(std::vector<int>& Arguments , int Count);
    ExpressionTokenizer TokenStream;

    int SubExp0();
    int SubExp1();
    int SubExp2();
    int SubExp3();
    int SubExp4();
    int SubExp5();
    int SubExp6();
    int SubExp7();
    int SubExp8();
    int SubExp9();
    int SubExp10();
    int SubExp11();
};

#endif // EXPRESSIONEVALUATOR_H
