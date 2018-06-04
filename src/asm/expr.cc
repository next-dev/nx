//----------------------------------------------------------------------------------------------------------------------
// Expression evaluator
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>

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

bool Expression::eval(Assembler& assembler, Lex& lex, MemAddr currentAddress)
{
    // #todo: introduce types (value, address, page, offset etc) into expressions.

    using T = Lex::Element::Type;

    //
    // Step 1 - convert to reverse polish notation using the Shunting Yard algorithm
    //
    vector<Value>   output;
    vector<Value>   opStack;

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

    for (const auto& v : m_queue)
    {
        switch (v.type)
        {
        case ValueType::UnaryOp:
        case ValueType::BinaryOp:
            // Operator
            while (
                !opStack.empty() &&
                (((opInfo[(int)v.elem->m_type - (int)T::Plus].assoc == Assoc::Left) &&
                (opInfo[(int)v.elem->m_type - (int)T::Plus].level == opInfo[(int)opStack.back().elem->m_type - (int)T::Plus].level)) ||
                    ((opInfo[(int)v.elem->m_type - (int)T::Plus].level > opInfo[(int)opStack.back().elem->m_type - (int)T::Plus].level))) &&
                opStack.back().type != ValueType::OpenParen)
            {
                output.emplace_back(opStack.back());
                opStack.pop_back();
            }
            // continue...

        case ValueType::OpenParen:
            opStack.emplace_back(v);
            break;

        case ValueType::Integer:
        case ValueType::Symbol:
        case ValueType::Char:
        case ValueType::Dollar:
            output.emplace_back(v);
            break;

        case ValueType::CloseParen:
            while (opStack.back().type != ValueType::OpenParen)
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
        assembler.error(lex, *v.elem, "Syntax error in expression.");   \
        return false;                                                   \
    } while(0)

    vector<ExprValue> stack;
    ExprValue a, b;

    for (const auto& v : output)
    {
        switch (v.type)
        {
        case ValueType::Integer:
        case ValueType::Char:
            stack.emplace_back(v.value);
            break;

        case ValueType::Symbol:
        {
            optional<MemAddr> value = assembler.lookUpLabel(v.value);
            if (value)
            {
                stack.emplace_back(*value);
            }
            else
            {
                optional<ExprValue> value = assembler.lookUpValue(v.value);
                if (value)
                {
                    stack.emplace_back(*value);
                }
                else
                {
                    assembler.error(lex, *v.elem, "Unknown symbol.");
                    return false;
                }
            }
        }
        break;

        case ValueType::Dollar:
            stack.emplace_back(currentAddress);
            break;

        case ValueType::UnaryOp:
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

        case ValueType::BinaryOp:
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
    m_result = stack[0];
    return true;
}

u16 Expression::make16(Assembler& assembler, Lex& lex, const Lex::Element& e, const Spectrum& speccy)
{
    switch (m_result.getType())
    {
    case ExprValue::Type::Integer:
        return r16();

    case ExprValue::Type::Address:
        if (speccy.isZ80Address(m_result))
        {
            return speccy.convertAddress(m_result);
        }
        else
        {
            assembler.error(lex, e, "Address expression is not viewable from the current Z80 bank configuration.");
            return 0;
        }
        break;
    default:
        assembler.error(lex, e, "Invalid 16-bit expression.");
        return 0;
    }
}

