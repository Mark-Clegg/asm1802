#ifndef ASSEMBLYEXPRESSIONEVALUATOR_H
#define ASSEMBLYEXPRESSIONEVALUATOR_H

#include <fmt/core.h>
#include <string>
#include "opcodetable.h"
#include "expressionevaluatorbase.h"
#include "symboltable.h"

class AssemblyExpressionEvaluator : public ExpressionEvaluatorBase
{
public:
    enum class FunctionEnum
    {
        FN_PROCESSOR,
        FN_HIGH,
        FN_LOW,
        FN_ISDEF,
        FN_ISNDEF
    };

    struct FunctionSpec
    {
        FunctionEnum ID;
        int Arguments;
    };

    AssemblyExpressionEvaluator(const SymbolTable& Global, uint16_t ProgramCounter, CPUTypeEnum Processor);
    void AddLocalSymbols(const SymbolTable* Local);

private:
    static const std::map<std::string, FunctionSpec> FunctionTable;
    const SymbolTable* Local;
    const SymbolTable* Global;
    bool LocalSymbols;      // Denotess if a local blob is available for symbol lookups
    CPUTypeEnum Processor;
    const uint16_t ProgramCounter;
    uint16_t SymbolValue(std::string Label);
    int AtomValue();
};

#endif // ASSEMBLYEXPRESSIONEVALUATOR_H
