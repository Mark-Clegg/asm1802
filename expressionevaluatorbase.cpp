#include "expressionexception.h"
#include "expressionevaluatorbase.h"

ExpressionEvaluatorBase::ExpressionEvaluatorBase()
{
}

int ExpressionEvaluatorBase::Evaluate(std::string& Expression)
{
    TokenStream.Initialize(Expression);
    int Result = EvaluateSubExpression();
    auto Token = TokenStream.Peek();
    if(Token != ExpressionTokenizer::TOKEN_END && Token != ExpressionTokenizer::TOKEN_CLOSE_BRACE)
        throw ExpressionException("Extra Characters at end of expression");
    return Result;
}

//!
//! \brief SubExp0
//! Lowest Precedence
//! Logical OR
//! \return
//!
int ExpressionEvaluatorBase::EvaluateSubExpression()
{
    int Result = SubExp1();
    while(TokenStream.Peek() == ExpressionTokenizer::TOKEN_LOGICAL_OR)
    {
        TokenStream.Get();
        int rhs = SubExp1();
        Result = ((Result != 0) || (rhs != 0))  ? 1 : 0;
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp1
//! Logical AND
//! \return
//!
int ExpressionEvaluatorBase::SubExp1()
{
    int Result = SubExp2();
    while(TokenStream.Peek() == ExpressionTokenizer::TOKEN_LOGICAL_AND)
    {
        TokenStream.Get();
        int rhs = SubExp2();
        Result = ((Result != 0) && (rhs != 0)) ? 1 : 0;
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp2
//! Bitwise OR
//! \return
//!
int ExpressionEvaluatorBase::SubExp2()
{
    int Result = SubExp3();
    while(TokenStream.Peek() == ExpressionTokenizer::TOKEN_BITWISE_OR)
    {
        TokenStream.Get();
        Result |= SubExp3();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp3
//! Bitwise XOR
//! \return
//!
int ExpressionEvaluatorBase::SubExp3()
{
    int Result = SubExp4();
    while(TokenStream.Peek() == ExpressionTokenizer::TOKEN_BITWISE_XOR)
    {
        TokenStream.Get();
        Result ^= SubExp4();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp4
//! Bitwise AND
//! \return
//!
int ExpressionEvaluatorBase::SubExp4()
{
    int Result = SubExp5();
    while(TokenStream.Peek() == ExpressionTokenizer::TOKEN_BITWISE_AND)
    {
        TokenStream.Get();
        Result &= SubExp5();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp5
//! EQUAL / NOT EQUAL
//! \return
//!
int ExpressionEvaluatorBase::SubExp5()
{
    int Result = SubExp6();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TOKEN_EQUAL || Token == ExpressionTokenizer::TOKEN_NOT_EQUAL)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TOKEN_EQUAL:
                Result = (Result == SubExp6()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TOKEN_NOT_EQUAL:
                Result = (Result == SubExp6()) ? 0 : 1;
                break;
            default:
                break;
        }
        Token = TokenStream.Peek();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp6
//! LESS / LESS or EQUAL / GREATER / GREATER or EQUAL
//! \return
//!
int ExpressionEvaluatorBase::SubExp6()
{
    int Result = SubExp7();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TOKEN_LESS || Token == ExpressionTokenizer::TOKEN_LESS_OR_EQUAL || Token == ExpressionTokenizer::TOKEN_GREATER || Token == ExpressionTokenizer::TOKEN_GREATER_OR_EQUAL)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TOKEN_LESS:
                Result = (Result < SubExp7()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TOKEN_LESS_OR_EQUAL:
                Result = (Result <= SubExp7()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TOKEN_GREATER:
                Result = (Result > SubExp7()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TOKEN_GREATER_OR_EQUAL:
                Result = (Result >= SubExp7()) ? 1 : 0;
                break;
            default:
                break;
        }
        Token = TokenStream.Peek();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp7
//! Shift LEFT / RIGHT
//! \return
//!
int ExpressionEvaluatorBase::SubExp7()
{
    int Result = SubExp8();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TOKEN_SHIFT_LEFT || Token == ExpressionTokenizer::TOKEN_SHIFT_RIGHT)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TOKEN_SHIFT_LEFT:
                Result <<= SubExp8();
                break;
            case ExpressionTokenizer::TOKEN_SHIFT_RIGHT:
                Result >>= SubExp8();
                break;
            default:
                break;
        }
        Token = TokenStream.Peek();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp8
//! ADDITION / SUPTRACTION
//! \return
//!
int ExpressionEvaluatorBase::SubExp8()
{
    int Result = SubExp9();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TOKEN_PLUS || Token == ExpressionTokenizer::TOKEN_MINUS)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TOKEN_PLUS:
                Result += SubExp9();
                break;
            case ExpressionTokenizer::TOKEN_MINUS:
                Result -= SubExp9();
                break;
            default:
                break;
        }
        Token = TokenStream.Peek();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp9
//! MULTIPLY / DIVIDE / REMAINDER
//! \return
//!
int ExpressionEvaluatorBase::SubExp9()
{
    int Result = SubExp10();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TOKEN_MULTIPLY || Token == ExpressionTokenizer::TOKEN_DIVIDE || Token == ExpressionTokenizer::TOKEN_REMAINDER)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TOKEN_MULTIPLY:
                Result *= SubExp10();
                break;
            case ExpressionTokenizer::TOKEN_DIVIDE:
            {
                int Operand = SubExp10();
                if(Operand == 0)
                    throw ExpressionException("Divide by zero");
                Result /= Operand;
                break;
            }
            case ExpressionTokenizer::TOKEN_REMAINDER:
            {
                int Operand = SubExp10();
                if(Operand == 0)
                    throw ExpressionException("Divide by zero");
                Result %= Operand;
                break;
            }
            default:
                break;
        }
        Token = TokenStream.Peek();
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp10
//! . Postfix operator - select Low ot High byte
//! \return
//!
int ExpressionEvaluatorBase::SubExp10()
{
    int Result = SubExp11();
    if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_DOT)
    {
        TokenStream.Get();
        int Selector = SubExp11();
        switch(Selector)
        {
            case 0:
                Result = Result & 0xFF;
                break;
            case 1:
                Result = (Result >> 8) & 0xFF;
                break;
            default:
                throw ExpressionException("Expected .0 or .1 High/Low selector");
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp11
//! Unary PLUS / MINUS / Bitwise NOT / Logical NOT
//! \return
//!
int ExpressionEvaluatorBase::SubExp11()
{
    auto Token = TokenStream.Peek();
    if(Token == ExpressionTokenizer::TOKEN_PLUS || Token == ExpressionTokenizer::TOKEN_MINUS || Token == ExpressionTokenizer::TOKEN_BITWISE_NOT || Token == ExpressionTokenizer::TOKEN_LOGICAL_NOT)
    {
        TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TOKEN_PLUS:
                return SubExp11();
                break;
            case ExpressionTokenizer::TOKEN_MINUS:
                return -SubExp11();
                break;
            case ExpressionTokenizer::TOKEN_BITWISE_NOT:
                return ~SubExp11();
                break;
            case ExpressionTokenizer::TOKEN_LOGICAL_NOT:
                return SubExp11() ? 1 : 0;
                break;
            default:
                break;
        }
    }
    return AtomValue();
}

//!
//! \brief ExpressionEvaluator::GetFunctionArguments
//! Evaluate function arguments and populate the passed vector.
//! \param Arguments
//! \param Count
//! \return True if correct number of arguments were found
//!
bool ExpressionEvaluatorBase::GetFunctionArguments(std::vector<int> &Arguments, int Count)
{
    if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_CLOSE_BRACE)
        TokenStream.Get();
    else
    {
        Arguments.push_back(EvaluateSubExpression());
        while(TokenStream.Peek() == ExpressionTokenizer::TOKEN_COMMA)
        {
            TokenStream.Get();
            Arguments.push_back(EvaluateSubExpression());
        }
        if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_CLOSE_BRACE)
            TokenStream.Get();
        else
            throw ExpressionException("Syntax error in argument list");
    }
    return Arguments.size() == Count;
}
