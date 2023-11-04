#include "preprocessorexpressionevaluator.h"
#include "preprocessorexception.h"

const std::map<std::string, PreProcessorExpressionEvaluator::FunctionSpec> PreProcessorExpressionEvaluator::FunctionTable =
{
    { "HIGH",        { FN_HIGH,    1 }},
    { "LOW",         { FN_LOW,     1 }}
};

PreProcessorExpressionEvaluator::PreProcessorExpressionEvaluator()
{

}

int PreProcessorExpressionEvaluator::Evaluate(std::string& Expression)
{
    TokenStream.Initialize(Expression);
    int Result = SubExp0();
    auto Token = TokenStream.Peek();
    if(Token != TOKEN_END && Token != TOKEN_CLOSE_BRACE)
        throw PreProcessorExpressionException("Extra Characters at end of expression");
    return Result;
}

//!
//! \brief SubExp0
//! Lowest Precedence
//! Logical OR
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp0()
{
    int Result = SubExp1();
    while(TokenStream.Peek() == TOKEN_LOGICAL_OR)
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
int PreProcessorExpressionEvaluator::SubExp1()
{
    int Result = SubExp2();
    while(TokenStream.Peek() == TOKEN_LOGICAL_AND)
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
int PreProcessorExpressionEvaluator::SubExp2()
{
    int Result = SubExp3();
    while(TokenStream.Peek() == TOKEN_BITWISE_OR)
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
int PreProcessorExpressionEvaluator::SubExp3()
{
    int Result = SubExp4();
    while(TokenStream.Peek() == TOKEN_BITWISE_XOR)
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
int PreProcessorExpressionEvaluator::SubExp4()
{
    int Result = SubExp5();
    while(TokenStream.Peek() == TOKEN_BITWISE_AND)
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
int PreProcessorExpressionEvaluator::SubExp5()
{
    int Result = SubExp6();
    while(TokenStream.Peek() == TOKEN_EQUAL || TokenStream.Peek() == TOKEN_NOT_EQUAL)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
            case TOKEN_EQUAL:
                Result = (Result == SubExp6()) ? 1 : 0;
                break;
            case TOKEN_NOT_EQUAL:
                Result = (Result == SubExp6()) ? 0 : 1;
                break;
            default:
                break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp6
//! LESS / LESS or EQUAL / GREATER / GREATER or EQUAL
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp6()
{
    int Result = SubExp7();
    while(TokenStream.Peek() == TOKEN_LESS || TokenStream.Peek() == TOKEN_LESS_OR_EQUAL || TokenStream.Peek() == TOKEN_GREATER || TokenStream.Peek() == TOKEN_GREATER_OR_EQUAL)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
            case TOKEN_LESS:
                Result = (Result < SubExp7()) ? 1 : 0;
                break;
            case TOKEN_LESS_OR_EQUAL:
                Result = (Result <= SubExp7()) ? 1 : 0;
                break;
            case TOKEN_GREATER:
                Result = (Result > SubExp7()) ? 1 : 0;
                break;
            case TOKEN_GREATER_OR_EQUAL:
                Result = (Result >= SubExp7()) ? 1 : 0;
                break;
            default:
                break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp7
//! Shift LEFT / RIGHT
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp7()
{
    int Result = SubExp8();
    while(TokenStream.Peek() == TOKEN_SHIFT_LEFT || TokenStream.Peek() == TOKEN_SHIFT_RIGHT)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
            case TOKEN_SHIFT_LEFT:
                Result <<= SubExp8();
                break;
            case TOKEN_SHIFT_RIGHT:
                Result >>= SubExp8();
                break;
            default:
                break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp8
//! ADDITION / SUPTRACTION
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp8()
{
    int Result = SubExp9();
    while(TokenStream.Peek() == TOKEN_PLUS || TokenStream.Peek() == TOKEN_MINUS)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
            case TOKEN_PLUS:
                Result += SubExp9();
                break;
            case TOKEN_MINUS:
                Result -= SubExp9();
                break;
            default:
                break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp9
//! MULTIPLY / DIVIDE / REMAINDER
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp9()
{
    int Result = SubExp10();
    while(TokenStream.Peek() == TOKEN_MULTIPLY || TokenStream.Peek() == TOKEN_DIVIDE || TokenStream.Peek() == TOKEN_REMAINDER)
    {
        auto Token = TokenStream.Get();
        switch(Token)
        {
            case TOKEN_MULTIPLY:
                Result *= SubExp10();
                break;
            case TOKEN_DIVIDE:
            {
                int Operand = SubExp10();
                if(Operand == 0)
                    throw PreProcessorExpressionException("Divide by zero");
                Result /= Operand;
                break;
            }
            case TOKEN_REMAINDER:
            {
                int Operand = SubExp10();
                if(Operand == 0)
                    throw PreProcessorExpressionException("Divide by zero");
                Result %= Operand;
                break;
            }
            default:
                break;
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp10
//! . Postfix operator - select Low ot High byte
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp10()
{
    int Result = SubExp11();
    if(TokenStream.Peek() == TOKEN_DOT)
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
                throw PreProcessorExpressionException("Expected .0 or .1 High/Low selector");
        }
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::SubExp11
//! Unary PLUS / MINUS / Bitwise NOT / Logical NOT
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp11()
{
    int Result;
    auto Token = TokenStream.Peek();
    if(Token == TOKEN_PLUS || Token == TOKEN_MINUS || Token == TOKEN_BITWISE_NOT || Token == TOKEN_LOGICAL_NOT)
    {
        TokenStream.Get();
        switch(Token)
        {
            case TOKEN_PLUS:
                return SubExp11();
                break;
            case TOKEN_MINUS:
                return -SubExp11();
                break;
            case TOKEN_BITWISE_NOT:
                return ~SubExp11();
                break;
            case TOKEN_LOGICAL_NOT:
                return SubExp11() ? 1 : 0;
                break;
            default:
                break;
        }
    }
    return SubExp12();
}

//!
//! \brief ExpressionEvaluator::SubExp9
//! Highest Precedence
//! Constant / Label / Function Call / Bracketed Expression
//! \return
//!
int PreProcessorExpressionEvaluator::SubExp12()
{
    int Result = 0;
    auto Token = TokenStream.Get();
    switch(Token)
    {
        case TOKEN_NUMBER:
            Result = TokenStream.IntegerValue;
            break;

        case TOKEN_OPEN_BRACE: // Bracketed Expression
            Result = SubExp0();
            if (TokenStream.Peek() != TOKEN_CLOSE_BRACE)
                throw PreProcessorExpressionException("Expected ')'");
            else
                TokenStream.Get();
            break;

        case TOKEN_LABEL:
        {
            std::string Label = TokenStream.StringValue;
            if(TokenStream.Peek() == TOKEN_OPEN_BRACE)
            {
                TokenStream.Get();

                auto FunctionSpec = FunctionTable.find(Label);
                if(FunctionSpec == FunctionTable.end())
                    throw PreProcessorExpressionException("Unknown function call");

                std::vector<int> Arguments = { };
                switch(FunctionSpec->second.ID)
                {
                    case FN_LOW:
                        if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                            throw PreProcessorExpressionException("Incorrect number of arguments: LOW expects 1 argument");
                        Result = Arguments[0] & 0xFF;
                        break;
                    case FN_HIGH:
                        if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                            throw PreProcessorExpressionException("Incorrect number of arguments: HIGH expects 1 argument");
                        Result = (Arguments[0] >> 8) & 0xFF;
                        break;
                }
            }
            else
                Result = 0; // Undefined Variable Name has value 0
            break;
        }
        default: // Should never happen
            throw PreProcessorExpressionException("Unsuppoerted operation in expression");
    }
    return Result;
}

//!
//! \brief ExpressionEvaluator::GetFunctionArguments
//! Evaluate function arguments and populate the passed vector.
//! \param Arguments
//! \param Count
//! \return True if correct number of arguments were found
//!
bool PreProcessorExpressionEvaluator::GetFunctionArguments(std::vector<int> &Arguments, int Count)
{
    if(TokenStream.Peek() == TOKEN_CLOSE_BRACE)
        TokenStream.Get();
    else
    {
        Arguments.push_back(SubExp0());
        while(TokenStream.Peek() == TOKEN_COMMA)
        {
            TokenStream.Get();
            Arguments.push_back(SubExp0());
        }
        if(TokenStream.Peek() == TOKEN_CLOSE_BRACE)
            TokenStream.Get();
        else
            throw PreProcessorExpressionException("Syntax error in argument list");
    }
    return Arguments.size() == Count;
}
