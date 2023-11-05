#ifndef ASSEMBLYEXPRESSIONEVALUATOR_H
#define ASSEMBLYEXPRESSIONEVALUATOR_H

#include <fmt/core.h>
#include <string>
#include "assemblyexpressiontokenizer.h"
#include "symboltable.h"

class AssemblyExpressionEvaluator
{
public:
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

    AssemblyExpressionEvaluator(const SymbolTable& Global, uint16_t ProgramCounter);
    void AddLocalSymbols(const SymbolTable* Local);
    int Evaluate(std::string& Expression);

private:
    static const std::map<std::string, FunctionSpec> FunctionTable;
    const SymbolTable* Local;
    const SymbolTable* Global;
    bool LocalSymbols;      // Denotess if a local blob is available for symbol lookups

    const uint16_t ProgramCounter;
    uint16_t SymbolValue(std::string Label);
    bool GetFunctionArguments(std::vector<int>& Arguments, int Count);
    AssemblyExpressionTokenizer TokenStream;

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
    int SubExp12();
};

#endif // ASSEMBLYEXPRESSIONEVALUATOR_H
