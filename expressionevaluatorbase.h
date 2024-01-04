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
    long Evaluate(std::string& Expression);

protected:
    bool GetFunctionArguments(std::vector<long>& Arguments, int Count);
    ExpressionTokenizer TokenStream;

private:
    long SubExp1();
    long SubExp2();
    long SubExp3();
    long SubExp4();
    long SubExp5();
    long SubExp6();
    long SubExp7();
    long SubExp8();
    long SubExp9();
    long SubExp10();
    long SubExp11();
protected:
    long EvaluateSubExpression();
    virtual long AtomValue() = 0;
};

#endif // EXPRESSIONEVALUATORBASE_H
