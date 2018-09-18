//----------------------------------------------------------------------------------------------------------------------
// Expression evaluator
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <asm/errors.h>
#include <emulator/memory.h>

#include <functional>
#include <map>
#ifdef __APPLE__
#   include <experimental/optional>
#else
#   include <optional>
#endif
#include <variant>

class Memory;

//----------------------------------------------------------------------------------------------------------------------
// ExprValue
// Stores an expression's value and type
//----------------------------------------------------------------------------------------------------------------------

class ExprValue
{
public:
    ExprValue();
    explicit ExprValue(i64 value);
    explicit ExprValue(MemAddr value);

    ExprValue(const ExprValue& other);
    ExprValue& operator= (const ExprValue& other);

    enum class Type
    {
        Invalid,
        Integer,
        Address,
    };

    Type getType() const { return m_type; }
    i64 getInteger() const { return get<i64>(m_value); }
    MemAddr getAddress() const { return get<MemAddr>(m_value); }

    //
    // Operations
    //

    static ExprValue ExprValue::opAdd(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opSub(const Memory& memory, const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opMul(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opDiv(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opMod(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opOr(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opAnd(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opXor(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opShiftLeft(const ExprValue& a, const ExprValue& b);
    static ExprValue ExprValue::opShiftRight(const ExprValue& a, const ExprValue& b);

    //
    // Convenience casts
    //
    u8 r8() const { assert(m_type == Type::Integer); return u8(get<i64>(m_value)); }
    u16 r16() const { assert(m_type == Type::Integer); return u16(get<i64>(m_value)); }

private:
    Type m_type;
    variant<i64, MemAddr> m_value;
};

//----------------------------------------------------------------------------------------------------------------------
// Represents an expression (calculated or not calculated)
//----------------------------------------------------------------------------------------------------------------------

class Expression
{
public:
    Expression();

    enum class ValueType
    {
        UnaryOp,
        BinaryOp,
        OpenParen,
        CloseParen,
        Integer,
        Symbol,
        Char,
        Dollar,
    };

    struct Value
    {
        ValueType               type;
        i64                     value;
        const Lex::Element*     elem;       // The element that described the operand

        Value(ValueType type, i64 value, const Lex::Element* e) : type(type), value(value), elem(e) {}
    };

    void addValue(ValueType type, i64 value, const Lex::Element* e);
    void addUnaryOp(Lex::Element::Type op, const Lex::Element* e);
    void addBinaryOp(Lex::Element::Type op, const Lex::Element* e);
    void addOpen(const Lex::Element* e);
    void addClose(const Lex::Element* e);


    const vector<Value>& getQueue() const { return m_queue; }

private:
    vector<Value>   m_queue;
};


//----------------------------------------------------------------------------------------------------------------------
// Expression evaluator
//----------------------------------------------------------------------------------------------------------------------

class ExpressionEvaluator
{
public:
    using ValueTable = map<i64, ExprValue>;
    using LabelTable = map<i64, MemAddr>;

    ExpressionEvaluator();

    void clear();

    // Access to the symbols table
    StringTable& getSymbols() { return m_symbols; }
    const StringTable& getSymbols() const { return m_symbols; }
    string getSymbol(i64 symbol) const;

    // Values
    bool addValue(i64 symbol, ExprValue value);
    optional<ExprValue> getValue(i64 symbol) const;
    void enumerateValues(function<void(string name, ExprValue value)> fn) const;

    // Labels
    bool addLabel(i64 symbol, MemAddr addr);
    optional<MemAddr> getLabel(i64 symbol) const;
    void enumerateLabels(function<void(string name, MemAddr addr)> fn) const;

    optional<ExprValue> parseExpression(const vector<u8>& text);
    optional<ExprValue> parseExpression(Lex& lex, ErrorManager& errs, const Memory& memory, const Lex::Element*& e, MemAddr currentAddress);

    // Find the end of the expression
    static void skipExpression(const Lex::Element*& e);

private:
    // Converts lexical tokens into an Expression (which contains expression tokens ready for parsing).  The token
    // stream must have already been checked for syntax correctness.
    static Expression constructExpression(const Lex::Element*& e);

    optional<ExprValue> eval(Lex& lex, ErrorManager& errs, const Memory& memory, MemAddr currentAddress, Expression& expr);

private:
    StringTable             m_symbols;      // Symbols generated by the lexical analysis
    ValueTable              m_values;
    LabelTable              m_labels;       // Labels are kept in a different namespace to values (which change).
};
