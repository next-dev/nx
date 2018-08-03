//----------------------------------------------------------------------------------------------------------------------
// Z80 Disassembler
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <config.h>
#include <types.h>
#include <asm/lex.h>
#include <emulator/spectrum.h>

#include <string>
#include <vector>
#include <optional>

enum class OperandType
{
    None,                   // No operand exists
    Expression,             // A valid expression
    AddressedExpression,    // A valid address expression (i.e. (nnnn)).
    IX_Expression,          // (IX+nn)
    IY_Expression,          // (IY+nn)

    A,
    B,
    C,
    D,
    E,
    H,
    L,
    I,
    R,
    AF,
    AF_,
    BC,
    DE,
    HL,
    IX,
    IY,
    IXH,
    IXL,
    IYH,
    IYL,
    SP,
    NC,
    Z,
    NZ,
    PO,
    PE,
    M,
    P,
    Address_BC,
    Address_DE,
    Address_HL,
    Address_SP,
    Address_C,

    // Used by the disassembler
    Expression8,
    Expression16,
    AddressedExpression8,
    Expression4,
    F,
};



class Disassembler
{
public:
    using LabelInfo = pair<string, MemAddr>;
    using Addresses = map<MemAddr, LabelInfo>;

    Disassembler()
    {

    }

    Disassembler(const Disassembler& other)
        : m_srcAddr(other.m_srcAddr)
        , m_opCode(other.m_opCode)
        , m_opCode2(other.m_opCode2)
        , m_comment(other.m_comment)
        , m_bytes(other.m_bytes)
    {
        for (int i = 0; i < 2; ++i) m_operands[i] = other.m_operands[i];
    }

    Disassembler& operator=(const Disassembler& other)
    {
        m_srcAddr = other.m_srcAddr;
        m_opCode = other.m_opCode;
        m_opCode2 = other.m_opCode2;
        for (int i = 0; i < 2; ++i) m_operands[i] = other.m_operands[i];
        m_comment = other.m_comment;
        m_bytes = other.m_bytes;

        return *this;
    }

    Disassembler(Disassembler&& other)
        : m_srcAddr(other.m_srcAddr)
        , m_opCode(other.m_opCode)
        , m_opCode2(other.m_opCode2)
        , m_comment(move(other.m_comment))
        , m_bytes(move(other.m_bytes))
    {
        for (int i = 0; i < 2; ++i) m_operands[i] = other.m_operands[i];
    }

    u16 disassemble(u16 a, u8 b1, u8 b2, u8 b3, u8 b4);
    std::string addressAndBytes(u16 a) const;
    std::string opCodeString() const;
    std::string operandString(const Spectrum& speccy, const Addresses& addresses) const;

    Lex::Element::Type  opCodeValue() const { return m_opCode; }
    Lex::Element::Type  opCode2Value() const { return m_opCode2; }
    OperandType         operand1Value() const { return m_operands[0].type; }
    OperandType         operand2Value() const { return m_operands[1].type; }
    i64                 param1Value() const { return m_operands[0].param; }
    i64                 param2Value() const { return m_operands[1].param; }
    const vector<u8>&   bytes() const { return m_bytes; }
    u16                 srcZ80Addr() const { return m_srcAddr; }

    optional<u16>       extractAddress() const;

    enum class DisplayType
    {
        Decimal,
        Hexadecimal,
        Binary,
        Equ,
        Base,
        Label,
    };

    struct Operand
    {
        OperandType     type;
        i64             param;
        DisplayType     displayMode;
        i64             symbol;

        Operand()
            : type(OperandType::None)
            , param(0)
            , displayMode(DisplayType::Hexadecimal)
            , symbol(0)
        {}

        Operand(const Operand& op)
            : type(op.type)
            , param(op.param)
            , displayMode(op.displayMode)
            , symbol(op.symbol)
        {}
    };

private:
    void result(Lex::Element::Type opCode, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, OperandType op2, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, i64 value1, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, i64 value1, OperandType op2, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, OperandType op2, i64 value2, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, i64 value1, OperandType op2, i64 value2, int instructionSize);
    void result(Lex::Element::Type opCode, Lex::Element::Type opCode2, OperandType op1, i64 value1, OperandType op2, i64 value2, int instructionSize);

//     void result(std::string&& opCode, std::string&& operands, int instructionSize);
//     void result(std::string&& opCode, int instructionSize);
    void invalidOpCode();

    void decode(u8 opCode, u8& x, u8& y, u8& z, u8& p, u8& q);

    static std::string opCodeString(Lex::Element::Type type);
    static std::string operandString(Operand op, Lex::Element::Type opCode2, i64 param2,
        const Spectrum& speccy, const Addresses& addresses);
    std::string operand1String(const Spectrum& speccy, const Addresses& addresses) const;
    std::string operand2String(const Spectrum& speccy, const Addresses& addresses) const;

    i64 byte(u8 b) const { return (i64)b; }
    i64 word(u8 l, u8 h) const { return (i64)l + 256 * (i64)h; }
    std::string wordString(u8 l, u8 h) const;
    std::string byteString(u8 b) const;
    std::string index(u8 b) const;
    i64 displacement(u16 a, u8 d, int instructionSize) const;
    std::string indexDisplacement(u8 d, std::string ix) const;
    std::string wordNoPrefix(u8 l, u8 h) const;
    std::string byteNoPrefix(u8 b) const;

    OperandType regs8(u8 y) const;
    OperandType regs16_1(u8 p) const;
    OperandType regs16_2(u8 p) const;
    OperandType regs8_ix(u8 y, OperandType ix) const;
    OperandType regs16_1_ix(u8 p, OperandType ix) const;
    OperandType regs16_2_ix(u8 p, OperandType ix) const;
    OperandType flags(u8 y) const;
    Lex::Element::Type aluOpCode(u8 y) const;
    bool aluOperandPrefix(u8 y) const;
    Lex::Element::Type rotShift(u8 y) const;
    OperandType ixh(OperandType ix) const { return ix == OperandType::IX ? OperandType::IXH : OperandType::IYH; }
    OperandType ixl(OperandType ix) const { return ix == OperandType::IX ? OperandType::IXL : OperandType::IYL; }
    OperandType ixExpr(OperandType ix) const { return ix == OperandType::IX ? OperandType::IX_Expression : OperandType::IY_Expression; }

    void disassembleCB(u8 b2);
    void disassembleDDFD(u8 b1, u8 b2, u8 b3, u8 b4, OperandType ix);
    void disassembleDDFDCB(u8 b3, u8 b4, OperandType ix);
    void disassembleED(u8 b2, u8 b3, u8 b4);

    void setDisplayType(int index, DisplayType type, i64 symbol);

private:
    // Formats are:
    //
    //      OPCODE OPERAND1
    //      OPCODE OPERAND1,OPERAND2
    //      OPCODE OPERAND1,OPCODE2 OPERAND2            e.g. ld b,rrc (ix+0)
    //      OPCODE OPERAND1,OPCODE2 PARAM1,OPERAND2     e.g. ld b,res 0,(ix+0)
    //
    
    // Derived data
    u16                 m_srcAddr;
    Lex::Element::Type  m_opCode;
    Lex::Element::Type  m_opCode2;
    Operand             m_operands[2];

    // Attached comment
    std::string         m_comment;

    // Original bytes
    std::vector<u8>     m_bytes;
};


//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

