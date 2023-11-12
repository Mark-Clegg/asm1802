#ifndef PREPROCESSOREXPRESSIONEVALUATOR_H
#define PREPROCESSOREXPRESSIONEVALUATOR_H

#include "expressionevaluatorbase.h"
#include "opcodetable.h"
#include <map>
#include <string>
#include <vector>

class PreProcessorExpressionEvaluator : public ExpressionEvaluatorBase
{
public:
    enum FunctionEnum
    {
        FN_HIGH,
        FN_LOW,
        FN_PROCESSOR
    };

    struct FunctionSpec
    {
        FunctionEnum ID;
        int Arguments;
    };

    PreProcessorExpressionEvaluator(const CPUTypeEnum Processor);
private:
    static const std::map<std::string, FunctionSpec> FunctionTable;
    const CPUTypeEnum Processor;
    int AtomValue();

};

#endif // PREPROCESSOREXPRESSIONEVALUATOR_H
