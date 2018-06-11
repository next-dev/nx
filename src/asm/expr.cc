//----------------------------------------------------------------------------------------------------------------------
// Expression evaluator
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <utils/format.h>

//----------------------------------------------------------------------------------------------------------------------
// Expression
//----------------------------------------------------------------------------------------------------------------------

Expression::Expression()
{

}

void Expression::addValue(ValueType type, i64 value, const Lex::Element* e)
{
    NX_ASSERT(type == ValueType::Integer ||
        type == ValueType::Symbol ||
        type == ValueType::Char ||
        type == ValueType::Dollar);
    m_queue.emplace_back(type, value, e);
}

void Expression::addUnaryOp(Lex::Element::Type op, const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::UnaryOp, (i64)op, e);
}

void Expression::addBinaryOp(Lex::Element::Type op, const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::BinaryOp, (i64)op, e);
}

void Expression::addOpen(const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::OpenParen, 0, e);
}

void Expression::addClose(const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::CloseParen, 0, e);
}

//----------------------------------------------------------------------------------------------------------------------
// ExpressionEvaluator
//----------------------------------------------------------------------------------------------------------------------

ExpressionEvaluator::ExpressionEvaluator()
{

}

optional<ExprValue> ExpressionEvaluator::parseExpression(const vector<u8>& text)
{
    return {};
}

optional<ExprValue> ExpressionEvaluator::parseExpression(Lex& lex, ErrorManager& errs, const Lex::Element*& e, MemAddr currentAddress)
{
    Expression expr = constructExpression(e);
    return eval(lex, errs, currentAddress, expr);
}

Expression ExpressionEvaluator::constructExpression(const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    int parenDepth = 0;
    int state = 0;
    Expression expr;

    for (;;)
    {
        switch (state)
        {
        case 0:
            switch (e->m_type)
            {
            case T::OpenParen:
                expr.addOpen(e);
                ++parenDepth;
                break;

            case T::Dollar:     expr.addValue(Expression::ValueType::Dollar, 0, e);                 state = 1;  break;
            case T::Symbol:     expr.addValue(Expression::ValueType::Symbol, e->m_symbol, e);       state = 1;  break;
            case T::Integer:    expr.addValue(Expression::ValueType::Integer, e->m_integer, e);     state = 1;  break;
            case T::Char:       expr.addValue(Expression::ValueType::Char, e->m_integer, e);        state = 1;  break;

            case T::Plus:       expr.addUnaryOp(Lex::Element::Type::Unary_Plus, e);                 state = 2;  break;
            case T::Minus:      expr.addUnaryOp(Lex::Element::Type::Unary_Minus, e);                state = 2;  break;
            case T::Tilde:      expr.addUnaryOp(Lex::Element::Type::Tilde, e);                      state = 2;  break;

            default:
                // Should never reach here!
                NX_ASSERT(0);
            }
            break;

        case 1:
            switch (e->m_type)
            {
            case T::Plus:
            case T::Minus:
            case T::LogicOr:
            case T::LogicAnd:
            case T::LogicXor:
            case T::ShiftLeft:
            case T::ShiftRight:
            case T::Multiply:
            case T::Divide:
            case T::Mod:
                expr.addBinaryOp(e->m_type, e);
                state = 0;
                break;

            case T::Comma:
                NX_ASSERT(parenDepth == 0);
                return expr;

            case T::Newline:
                NX_ASSERT(parenDepth == 0);
                return expr;

            case T::CloseParen:
                if (parenDepth > 0)
                {
                    --parenDepth;
                    expr.addClose(e);
                }
                else
                {
                    return expr;
                }
                break;

            default:
                NX_ASSERT(0);
            }
            break;

        case 2:
            switch (e->m_type)
            {
            case T::Dollar:     expr.addValue(Expression::ValueType::Dollar, 0, e);              state = 1;  break;
            case T::Symbol:     expr.addValue(Expression::ValueType::Symbol, e->m_symbol, e);    state = 1;  break;
            case T::Integer:    expr.addValue(Expression::ValueType::Integer, e->m_integer, e);  state = 1;  break;
            case T::Char:       expr.addValue(Expression::ValueType::Char, e->m_integer, e);     state = 1;  break;
            case T::OpenParen:
                expr.addOpen(e);
                ++parenDepth;
                state = 0;
                break;
            default:
                NX_ASSERT(0);
            }
        }

        ++e;
    }
}

void ExpressionEvaluator::skipExpression(const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    int parenDepth = 0;
    int state = 0;

    for (;;)
    {
        switch (state)
        {
        case 0:
            switch (e->m_type)
            {
            case T::OpenParen:
                ++parenDepth;
                break;

            case T::Dollar:
            case T::Symbol:
            case T::Integer:
            case T::Char:
                state = 1;
                break;

            case T::Plus:
            case T::Minus:
            case T::Tilde:
                state = 2;
                break;

            default:
                // Should never reach here!
                NX_ASSERT(0);
            }
            break;

        case 1:
            switch (e->m_type)
            {
            case T::Plus:
            case T::Minus:
            case T::LogicOr:
            case T::LogicAnd:
            case T::LogicXor:
            case T::ShiftLeft:
            case T::ShiftRight:
            case T::Multiply:
            case T::Divide:
            case T::Mod:
                state = 0;
                break;

            case T::Comma:
                NX_ASSERT(parenDepth == 0);
                return;

            case T::Newline:
                NX_ASSERT(parenDepth == 0);
                return;

            case T::CloseParen:
                if (parenDepth > 0)
                {
                    --parenDepth;
                }
                else
                {
                    return;
                }
                break;

            default:
                NX_ASSERT(0);
            }
            break;

        case 2:
            switch (e->m_type)
            {
            case T::Dollar:
            case T::Symbol:
            case T::Integer:
            case T::Char:
                state = 1;
                break;

            case T::OpenParen:
                ++parenDepth;
                state = 0;
                break;
            default:
                NX_ASSERT(0);
            }
        }

        ++e;
    }
}

optional<ExprValue> ExpressionEvaluator::eval(Lex& lex, ErrorManager& errs, MemAddr currentAddress, Expression& expr)
{
    // #todo: introduce types (value, address, page, offset etc) into expressions.

    using T = Lex::Element::Type;

    //
    // Step 1 - convert to reverse polish notation using the Shunting Yard algorithm
    //
    vector<Expression::Value>   output;
    vector<Expression::Value>   opStack;

    enum class Assoc
    {
        Left,
        Right,
    };

    struct OpInfo
    {
        int     level;
        Assoc   assoc;
    };

    // Operator precedence:
    //
    //      0:  - + ~ (unary ops)
    //      1:  * / %
    //      2:  + -
    //      3:  << >>
    //      4:  &
    //      5:  | ^

    OpInfo opInfo[] = {
        { 2, Assoc::Left },         // Plus
        { 2, Assoc::Left },         // Minus
        { 5, Assoc::Left },         // LogicOr
        { 4, Assoc::Left },         // LogicAnd
        { 5, Assoc::Left },         // LogicXor
        { 3, Assoc::Left },         // ShiftLeft,
        { 3, Assoc::Left },         // ShiftRight,
        { 0, Assoc::Right },        // Unary tilde
        { 1, Assoc::Left },         // Multiply
        { 1, Assoc::Left },         // Divide
        { 1, Assoc::Left },         // Mod
        { 0, Assoc::Right },        // Unary plus
        { 0, Assoc::Right },        // Unary minus
    };

    for (const auto& v : expr.getQueue())
    {
        switch (v.type)
        {
        case Expression::ValueType::UnaryOp:
        case Expression::ValueType::BinaryOp:
            // Operator
            while (
                !opStack.empty() &&
                (((opInfo[(int)v.elem->m_type - (int)T::Plus].assoc == Assoc::Left) &&
                (opInfo[(int)v.elem->m_type - (int)T::Plus].level == opInfo[(int)opStack.back().elem->m_type - (int)T::Plus].level)) ||
                    ((opInfo[(int)v.elem->m_type - (int)T::Plus].level > opInfo[(int)opStack.back().elem->m_type - (int)T::Plus].level))) &&
                opStack.back().type != Expression::ValueType::OpenParen)
            {
                output.emplace_back(opStack.back());
                opStack.pop_back();
            }
            // continue...

        case Expression::ValueType::OpenParen:
            opStack.emplace_back(v);
            break;

        case Expression::ValueType::Integer:
        case Expression::ValueType::Symbol:
        case Expression::ValueType::Char:
        case Expression::ValueType::Dollar:
            output.emplace_back(v);
            break;

        case Expression::ValueType::CloseParen:
            while (opStack.back().type != Expression::ValueType::OpenParen)
            {
                output.emplace_back(opStack.back());
                opStack.pop_back();
            }
            opStack.pop_back();
            break;
        }

    }

    while (!opStack.empty())
    {
        output.emplace_back(opStack.back());
        opStack.pop_back();
    }

    //
    // Step 2 - Execute the RPN expression
    // If this fails, the expression cannot be evaluated yet return false after outputting a message
    //

#define FAIL()                                                          \
    do {                                                                \
        errs.error(lex, *v.elem, "Syntax error in expression.");             \
        return {};                                                      \
    } while(0)

    vector<ExprValue> stack;
    ExprValue a, b;

    for (const auto& v : output)
    {
        switch (v.type)
        {
        case Expression::ValueType::Integer:
        case Expression::ValueType::Char:
            stack.emplace_back(v.value);
            break;

        case Expression::ValueType::Symbol:
        {
            optional<MemAddr> value = getLabel(v.value);
            if (value)
            {
                stack.emplace_back(*value);
            }
            else
            {
                optional<ExprValue> value = getValue(v.value);
                if (value)
                {
                    stack.emplace_back(*value);
                }
                else
                {
                    errs.error(lex, *v.elem, "Unknown symbol.");
                    return {};
                }
            }
        }
        break;

        case Expression::ValueType::Dollar:
            stack.emplace_back(currentAddress);
            break;

        case Expression::ValueType::UnaryOp:
            if (stack.size() < 1) FAIL();
            switch ((T)v.value)
            {
            case T::Unary_Plus:
                // Do nothing
                break;

            case T::Unary_Minus:
                if (stack.back().getType() == ExprValue::Type::Integer)
                {
                    stack.back() = ExprValue(-stack.back());
                }
                else
                {
                    FAIL();
                }
                break;

            case T::Tilde:
                if (stack.back().getType() == ExprValue::Type::Integer)
                {
                    stack.back() = ExprValue(~stack.back());
                }
                else
                {
                    FAIL();
                }
                break;

            default:
                FAIL();
            }
            break;

        case Expression::ValueType::BinaryOp:
            if (stack.size() < 2) FAIL();
            b = stack.back();   stack.pop_back();
            a = stack.back();   stack.pop_back();
            switch ((T)v.value)
            {
            case T::Plus:       stack.emplace_back(a + b);      break;
            case T::Minus:      stack.emplace_back(a - b);      break;
            case T::LogicOr:    stack.emplace_back(a | b);      break;
            case T::LogicAnd:   stack.emplace_back(a & b);      break;
            case T::LogicXor:   stack.emplace_back(a ^ b);      break;
            case T::ShiftLeft:  stack.emplace_back(a << b);     break;
            case T::ShiftRight: stack.emplace_back(a >> b);     break;
            case T::Multiply:   stack.emplace_back(a * b);      break;
            case T::Divide:     stack.emplace_back(a / b);      break;
            case T::Mod:        stack.emplace_back(a % b);      break;
            default:
                FAIL();
            }
            break;

        default:
            NX_ASSERT(0);
            break;
        }
    }

#undef FAIL

    NX_ASSERT(stack.size() == 1);
    return stack[0];
}

void ExpressionEvaluator::clear()
{
    m_symbols.clear();
    m_values.clear();
    m_labels.clear();
}

optional<MemAddr> ExpressionEvaluator::getLabel(i64 symbol) const
{
    auto it = m_labels.find(symbol);
    return it == m_labels.end() ? optional<MemAddr>() : it->second;
}

optional<ExprValue> ExpressionEvaluator::getValue(i64 symbol) const
{
    auto it = m_values.find(symbol);
    return it == m_values.end() ? optional<ExprValue>() : it->second;
}

bool ExpressionEvaluator::addLabel(i64 symbol, MemAddr addr)
{
    auto it = m_labels.find(symbol);
    if (it == m_labels.end())
    {
        m_labels[symbol] = addr;
        return true;
    }

    return false;
}

bool ExpressionEvaluator::addValue(i64 symbol, ExprValue value)
{
    auto it = m_values.find(symbol);
    if (it == m_values.end())
    {
        m_values[symbol] = value;
        return true;
    }

    return false;
}

void ExpressionEvaluator::enumerateLabels(function<void(string name, MemAddr addr)> fn) const
{
    for (const auto& labelPair : m_labels)
    {
        fn(getSymbol(labelPair.first), labelPair.second);
    }
}

void ExpressionEvaluator::enumerateValues(function<void(string name, ExprValue value)> fn) const
{
    for (const auto& valuePair : m_values)
    {
        fn(getSymbol(valuePair.first), valuePair.second);
    }
}

string ExpressionEvaluator::getSymbol(i64 symbol) const
{
    const char* str = (const char *)m_symbols.get(symbol);
    return str ? str : "";
}
