//----------------------------------------------------------------------------------------------------------------------
// Z80 Disassembler
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "config.h"
#include "types.h"

#include <string>
#include <vector>

class Disassembler
{
public:
    u16 disassemble(u16 a, u8 b1, u8 b2, u8 b3, u8 b4);
    std::string addressAndBytes(u16 a);
    std::string opCode();
    std::string operands();

private:
    void result(std::string&& opCode, std::string&& operands, int instructionSize);
    void result(std::string&& opCode, int instructionSize);
    void invalidOpCode();

    void decode(u8 opCode, u8& x, u8& y, u8& z, u8& p, u8& q);

    std::string word(u8 l, u8 h);
    std::string byte(u8 b);
    std::string index(u8 b);
    std::string displacement(u16 a, u8 d, int instructionSize);
    std::string indexDisplacement(u8 d, std::string ix);
    std::string wordNoPrefix(u8 l, u8 h);
    std::string byteNoPrefix(u8 b);

    std::string regs8(u8 y);
    std::string regs16_1(u8 p);
    std::string regs16_2(u8 p);
    std::string regs8_ix(u8 y, std::string ix, u8 d);
    std::string regs16_1_ix(u8 p, std::string ix);
    std::string regs16_2_ix(u8 p, std::string ix);
    std::string flags(u8 y);
    std::string aluOpCode(u8 y);
    std::string aluOperandPrefix(u8 y);
    std::string rotShift(u8 y);

    void disassembleCB(u8 b2);
    void disassembleDDFD(u8 b1, u8 b2, u8 b3, u8 b4, std::string ix);
    void disassembleDDFDCB(u8 b3, u8 b4, std::string ix);
    void disassembleED(u8 b2, u8 b3, u8 b4);

private:
    std::string         m_opCode;
    std::string         m_operands;
    std::string         m_comment;
    std::vector<u8>     m_bytes;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

