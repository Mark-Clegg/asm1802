#ifndef EXPRESSIONEVALUATORBASE_H
#define EXPRESSIONEVALUATORBASE_H

#include <map>
#include <string>
#include <vector>
#include "expressiontokenizer.h"

class ExpressionEvaluatorBase
{
public:
    ExpressionEvaluatorBase();
    int Evaluate(std::string& Expression);

protected:
    bool GetFunctionArguments(std::vector<int>& Arguments, int Count);
    ExpressionTokenizer TokenStream;

private:
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
protected:
    int EvaluateSubExpression();
    virtual int AtomValue() = 0;
};

#endif // EXPRESSIONEVALUATORBASE_H
