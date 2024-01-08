#include "expressionexception.h"
#include "expressionevaluatorbase.h"

ExpressionEvaluatorBase::ExpressionEvaluatorBase()
{
}

long ExpressionEvaluatorBase::Evaluate(std::string& Expression)
{
    TokenStream.Initialize(Expression);
    long Result = EvaluateSubExpression();
    auto Token = TokenStream.Peek();
    if(Token != ExpressionTokenizer::TokenEnum::TOKEN_END && Token != ExpressionTokenizer::TokenEnum::TOKEN_CLOSE_BRACE)
        throw ExpressionException("Extra Characters at end of expression");
    return Result;
}

//!
//! \brief SubExp0
//! Lowest Precedence
//! Logical OR
//! \return
//!
long ExpressionEvaluatorBase::EvaluateSubExpression()
{
    long Result = SubExp1();
    while(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_LOGICAL_OR)
    {
        TokenStream.Get();
        long rhs = SubExp1();
        Result = ((Result != 0) || (rhs != 0))  ? 1 : 0;
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp1
//! Logical AND
//! \return
//!
long ExpressionEvaluatorBase::SubExp1()
{
    long Result = SubExp2();
    while(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_LOGICAL_AND)
    {
        TokenStream.Get();
        long rhs = SubExp2();
        Result = ((Result != 0) && (rhs != 0)) ? 1 : 0;
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp2
//! Bitwise OR
//! \return
//!
long ExpressionEvaluatorBase::SubExp2()
{
    long Result = SubExp3();
    while(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_BITWISE_OR)
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
long ExpressionEvaluatorBase::SubExp3()
{
    long Result = SubExp4();
    while(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_BITWISE_XOR)
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
long ExpressionEvaluatorBase::SubExp4()
{
    long Result = SubExp5();
    while(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_BITWISE_AND)
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
long ExpressionEvaluatorBase::SubExp5()
{
    long Result = SubExp6();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TokenEnum::TOKEN_EQUAL || Token == ExpressionTokenizer::TokenEnum::TOKEN_NOT_EQUAL)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TokenEnum::TOKEN_EQUAL:
                Result = (Result == SubExp6()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_NOT_EQUAL:
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
long ExpressionEvaluatorBase::SubExp6()
{
    long Result = SubExp7();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TokenEnum::TOKEN_LESS || Token == ExpressionTokenizer::TokenEnum::TOKEN_LESS_OR_EQUAL || Token == ExpressionTokenizer::TokenEnum::TOKEN_GREATER || Token == ExpressionTokenizer::TokenEnum::TOKEN_GREATER_OR_EQUAL)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TokenEnum::TOKEN_LESS:
                Result = (Result < SubExp7()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_LESS_OR_EQUAL:
                Result = (Result <= SubExp7()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_GREATER:
                Result = (Result > SubExp7()) ? 1 : 0;
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_GREATER_OR_EQUAL:
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
long ExpressionEvaluatorBase::SubExp7()
{
    long Result = SubExp8();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TokenEnum::TOKEN_SHIFT_LEFT || Token == ExpressionTokenizer::TokenEnum::TOKEN_SHIFT_RIGHT)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TokenEnum::TOKEN_SHIFT_LEFT:
                Result <<= SubExp8();
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_SHIFT_RIGHT:
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
long ExpressionEvaluatorBase::SubExp8()
{
    long Result = SubExp9();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TokenEnum::TOKEN_PLUS || Token == ExpressionTokenizer::TokenEnum::TOKEN_MINUS)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TokenEnum::TOKEN_PLUS:
                Result += SubExp9();
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_MINUS:
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
long ExpressionEvaluatorBase::SubExp9()
{
    long Result = SubExp10();
    auto Token = TokenStream.Peek();
    while(Token == ExpressionTokenizer::TokenEnum::TOKEN_MULTIPLY || Token == ExpressionTokenizer::TokenEnum::TOKEN_DIVIDE || Token == ExpressionTokenizer::TokenEnum::TOKEN_REMAINDER)
    {
        Token = TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TokenEnum::TOKEN_MULTIPLY:
                Result *= SubExp10();
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_DIVIDE:
            {
                long Operand = SubExp10();
                if(Operand == 0)
                    throw ExpressionException("Divide by zero");
                Result /= Operand;
                break;
            }
            case ExpressionTokenizer::TokenEnum::TOKEN_REMAINDER:
            {
                long Operand = SubExp10();
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
//! . Postfix operator - select byte
//! \return
//!
long ExpressionEvaluatorBase::SubExp10()
{
    long Result = SubExp11();
    if(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_DOT)
    {
        TokenStream.Get();
        long Selector = SubExp11();
        switch(Selector)
        {
            case 0:
                Result = Result & 0xFF;
                break;
            case 1:
                Result = (Result >> 8) & 0xFF;
                break;
            case 2:
                Result = (Result >> 16) & 0xFF;
                break;
            case 3:
                Result = (Result >> 24) & 0xFF;
                break;
            case 4:
                if(sizeof(long) < 8)
                    throw ExpressionException(".4 Not supported in this build");
                Result = (Result >> 32) & 0xFF;
                break;
            case 5:
                if(sizeof(long) < 8)
                    throw ExpressionException(".5 Not supported in this build");
                Result = (Result >> 40) & 0xFF;
                break;
            case 6:
                if(sizeof(long) < 8)
                    throw ExpressionException(".6 Not supported in this build");
                Result = (Result >> 48) & 0xFF;
                break;
            case 7:
                if(sizeof(long) < 8)
                    throw ExpressionException(".7 Not supported in this build");
                Result = (Result >> 56) & 0xFF;
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
long ExpressionEvaluatorBase::SubExp11()
{
    auto Token = TokenStream.Peek();
    if(Token == ExpressionTokenizer::TokenEnum::TOKEN_PLUS || Token == ExpressionTokenizer::TokenEnum::TOKEN_MINUS || Token == ExpressionTokenizer::TokenEnum::TOKEN_BITWISE_NOT || Token == ExpressionTokenizer::TokenEnum::TOKEN_LOGICAL_NOT)
    {
        TokenStream.Get();
        switch(Token)
        {
            case ExpressionTokenizer::TokenEnum::TOKEN_PLUS:
                return SubExp11();
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_MINUS:
                return -SubExp11();
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_BITWISE_NOT:
                return ~SubExp11();
                break;
            case ExpressionTokenizer::TokenEnum::TOKEN_LOGICAL_NOT:
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
bool ExpressionEvaluatorBase::GetFunctionArguments(std::vector<long> &Arguments, int Count)
{
    if(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_CLOSE_BRACE)
        TokenStream.Get();
    else
    {
        Arguments.push_back(EvaluateSubExpression());
        while(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_COMMA)
        {
            TokenStream.Get();
            Arguments.push_back(EvaluateSubExpression());
        }
        if(TokenStream.Peek() == ExpressionTokenizer::TokenEnum::TOKEN_CLOSE_BRACE)
            TokenStream.Get();
        else
            throw ExpressionException("Syntax error in argument list");
    }
    return Arguments.size() == Count;
}
