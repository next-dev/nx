//----------------------------------------------------------------------------------------------------------------------
// Z80 Disassembler
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <config.h>
#include <types.h>
#include <asm/lex.h>

#include <string>
#include <vector>

enum class OperandType
{
    None,                   // No operand exists
    Expression,             // A valid expression
    AddressedExpression,    // A valid address expression (i.e. (nnnn)).
    IX_Expression,
    IY_Expression,

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
    u16 disassemble(u16 a, u8 b1, u8 b2, u8 b3, u8 b4);
    std::string addressAndBytes(u16 a) const;
    std::string opCode() const;
    std::string operand1String() const;
    std::string operand2String() const;

    Lex::Element::Type  opCodeValue() const;
    OperandType         operand1Value() const;
    OperandType         operand2Value() const;

private:
    void result(Lex::Element::Type opCode, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, OperandType op2, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, i64 value1, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, i64 value1, OperandType op2, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, OperandType op2, i64 value2, int instructionSize);
    void result(Lex::Element::Type opCode, OperandType op1, i64 value1, OperandType op2, i64 value2, int instructionSize);

//     void result(std::string&& opCode, std::string&& operands, int instructionSize);
//     void result(std::string&& opCode, int instructionSize);
    void invalidOpCode();

    void decode(u8 opCode, u8& x, u8& y, u8& z, u8& p, u8& q);

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
    OperandType ixh(OperandType ix) const { return ix == O::IX ? O::IXH : O::IYH; }
    OperandType ixl(OperandType ix) const { return ix == O::IX ? O::IXL : O::IYL; }
    OperandType ixExpr(OperandType ix) const { return ix == O::IX ? O::IX_Expression : O::IY_Expression; }

    void disassembleCB(u8 b2);
    void disassembleDDFD(u8 b1, u8 b2, u8 b3, u8 b4, OperandType ix);
    void disassembleDDFDCB(u8 b3, u8 b4, OperandType ix);
    void disassembleED(u8 b2, u8 b3, u8 b4);

private:
    std::string         m_opCode;
    std::string         m_operands;
    std::string         m_comment;
    std::vector<u8>     m_bytes;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

