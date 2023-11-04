#ifndef PREPROCESSOREXPRESSIONEVALUATOR_H
#define PREPROCESSOREXPRESSIONEVALUATOR_H

#include "preprocessorexpressiontokenizer.h"
#include <map>
#include <string>
#include <vector>

class PreProcessorExpressionEvaluator
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
    int Evaluate(std::string& Expression);
private:
    PreProcessorExpressionTokenizer TokenStream;
    static const std::map<std::string, FunctionSpec> FunctionTable;

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
    bool GetFunctionArguments(std::vector<int> &Arguments, int Count);
};

#endif // PREPROCESSOREXPRESSIONEVALUATOR_H
