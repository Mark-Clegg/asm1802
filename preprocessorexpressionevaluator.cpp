#include "preprocessorexpressionevaluator.h"
#include "expressionexception.h"
#include "opcodetable.h"

const std::map<std::string, PreProcessorExpressionEvaluator::FunctionSpec> PreProcessorExpressionEvaluator::FunctionTable =
{
    { "HIGH",        { FN_HIGH,      1 }},
    { "LOW",         { FN_LOW,       1 }},
    { "CPU",         { FN_PROCESSOR, 1 }},
    { "PROCESSOR",   { FN_PROCESSOR, 1 }}
};

PreProcessorExpressionEvaluator::PreProcessorExpressionEvaluator(const CPUTypeEnum Processor) : ExpressionEvaluatorBase(), Processor(Processor)
{
}

//!
//! \brief ExpressionEvaluator::TokenValue
//! Highest Precedence
//! Constant / Label / Function Call / Bracketed Expression
//! \return
//!
int PreProcessorExpressionEvaluator::AtomValue()
{
    int Result = 0;
    auto Token = TokenStream.Get();
    switch(Token)
    {
        case ExpressionTokenizer::TOKEN_QUOTED_STRING:
            throw ExpressionException("Unexpected string literal");
            break;

        case ExpressionTokenizer::TOKEN_NUMBER:
            Result = TokenStream.IntegerValue;
            break;

        case ExpressionTokenizer::TOKEN_OPEN_BRACE: // Bracketed Expression
            Result = EvaluateSubExpression();
            if (TokenStream.Peek() != ExpressionTokenizer::TOKEN_CLOSE_BRACE)
                throw ExpressionException("Expected ')'");
            else
                TokenStream.Get();
            break;

        case ExpressionTokenizer::TOKEN_LABEL:
        {
            std::string Label = TokenStream.StringValue;
            if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_OPEN_BRACE)
            {
                TokenStream.Get();

                auto FunctionSpec = FunctionTable.find(Label);
                if(FunctionSpec == FunctionTable.end())
                    throw ExpressionException("Unknown function call");

                std::vector<int> Arguments = { };
                switch(FunctionSpec->second.ID)
                {
                    case FN_LOW:
                        if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                            throw ExpressionException("Incorrect number of arguments: LOW expects 1 argument");
                        Result = Arguments[0] & 0xFF;
                        break;
                    case FN_HIGH:
                        if(!GetFunctionArguments(Arguments, FunctionSpec->second.Arguments))
                            throw ExpressionException("Incorrect number of arguments: HIGH expects 1 argument");
                        Result = (Arguments[0] >> 8) & 0xFF;
                        break;
                    case FN_PROCESSOR:
                    {
                        auto Argument = TokenStream.Peek();
                        std::string Value;
                        if(Argument == ExpressionTokenizer::TOKEN_QUOTED_STRING)
                            TokenStream.Get();
                        else if(!TokenStream.GetCustomToken(std::regex(R"(^((CDP)?180[2456]A?).*)")))
                            throw ExpressionException("Expected Processor designation");

                        Value = TokenStream.StringValue;

                        if(TokenStream.Peek() == ExpressionTokenizer::TOKEN_CLOSE_BRACE)
                        {
                            TokenStream.Get();

                            auto CPU = OpCodeTable::CPUTable.find(Value);
                            if(CPU == OpCodeTable::CPUTable.end())
                                throw ExpressionException("Unrecognised processor designation");
                            Result = CPU->second <= Processor ? 1 : 0;
                        }
                        else
                            throw ExpressionException("Extra characters after quoted Processor designation");
                        break;
                    }
                }
            }
            else
                Result = 0; // Undefined Variable Name has value 0
            break;
        }
        default: // Should never happen
            throw ExpressionException("Unsuppoerted operation in expression");
    }
    return Result;
}
