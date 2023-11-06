#ifndef PREPROCESSOREXPRESSIONEVALUATOR_H
#define PREPROCESSOREXPRESSIONEVALUATOR_H

#include "expressionevaluatorbase.h"
#include <map>
#include <string>
#include <vector>

class PreProcessorExpressionEvaluator : public ExpressionEvaluatorBase
{
public:
    enum FunctionEnum
    {
        FN_HIGH,
        FN_LOW
    };

    struct FunctionSpec
    {
        FunctionEnum ID;
        int Arguments;
    };

    PreProcessorExpressionEvaluator();
private:
    static const std::map<std::string, FunctionSpec> FunctionTable;
    int AtomValue();

};

#endif // PREPROCESSOREXPRESSIONEVALUATOR_H
