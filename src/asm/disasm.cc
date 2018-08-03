//----------------------------------------------------------------------------------------------------------------------
// Z80 Disassembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/disasm.h>
#include <utils/format.h>

#include <cassert>
#include <iomanip>
#include <sstream>

using T = Lex::Element::Type;
using O = OperandType;

//----------------------------------------------------------------------------------------------------------------------
// Disassembler
//----------------------------------------------------------------------------------------------------------------------

void Disassembler::decode(u8 opCode, u8& x, u8& y, u8& z, u8& p, u8& q)
{
    x = (opCode & 0xc0) >> 6;
    y = (opCode & 0x38) >> 3;
    z = (opCode & 0x07);
    p = (y & 0x06) >> 1;
    q = (y & 0x01);
}

// void Disassembler::result(std::string&& opCode, std::string&& operands, int instructionSize)
// {
//     m_opCode = std::move(opCode);
//     m_operands = std::move(operands);
//     m_comment = std::string();
// 
//     for (int i = instructionSize; i < 4; ++i) m_bytes.pop_back();
// }
// 
// void Disassembler::result(std::string&& opCode, int instructionSize)
// {
//     result(std::move(opCode), std::string(), instructionSize);
// }

void Disassembler::invalidOpCode()
{
    result(T::DB, O::Expression, 0, 1);
}

std::string Disassembler::wordString(u8 l, u8 h) const
{
    std::stringstream ss;
    ss << '$'
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)h
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)l;
    return ss.str();
}

std::string Disassembler::byteString(u8 b) const
{
    std::stringstream ss;
    ss << '$' << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)b;
    return ss.str();
}

std::string Disassembler::wordNoPrefix(u8 l, u8 h) const
{
    std::stringstream ss;
    ss
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)h
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)l;
    return ss.str();
}

std::string Disassembler::byteNoPrefix(u8 b) const
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)b;
    return ss.str();
}

std::string Disassembler::index(u8 b) const
{
    std::stringstream ss;
    ss << (int)b;
    return ss.str();
}

i64 Disassembler::displacement(u16 a, u8 d, int instructionSize) const
{
    a += instructionSize + (int)(i8)d;
    return (i64)a;
}

std::string Disassembler::indexDisplacement(u8 d, std::string ix) const
{
    i8 x = (i8)d;
    return "(" + ix + (x < 0 ? "-" : "+") + byteString(x < 0 ? -d : d) + ")";
}

OperandType Disassembler::regs8(u8 y) const
{
    //static const char* regs[] = { "b", "c", "d", "e", "h", "l", "(hl)", "a" };
    static O regs[] = {
        O::B, O::C,
        O::D, O::E,
        O::H, O::L,
        O::Address_HL, O::A,
    };
    NX_ASSERT(y >= 0 && y < 8);
    return regs[y];
}

OperandType Disassembler::regs16_1(u8 p) const
{
    //static const char* regs[] = { "bc", "de", "hl", "sp" };
    static O regs[] = {
        O::BC,
        O::DE,
        O::HL,
        O::SP,
    };
    NX_ASSERT(p >= 0 && p < 4);
    return regs[p];
}

OperandType Disassembler::regs16_2(u8 p) const
{
    //static const char* regs[] = { "bc", "de", "hl", "af" };
    static O regs[] = { O::BC, O::DE, O::HL, O::AF };
    NX_ASSERT(p >= 0 && p < 4);
    return regs[p];
}

OperandType Disassembler::regs8_ix(u8 y, OperandType ix) const
{
    NX_ASSERT(y >= 0 && y < 8);
    switch (y)
    {
    case 0: return O::B; // "b";
    case 1: return O::C; // "c";
    case 2: return O::D; // "d";
    case 3: return O::E; // "e";
    case 4: return ixh(ix); // ix + "h";
    case 5: return ixl(ix); // ix + "l";
    case 6: return ixExpr(ix); // indexDisplacement(d, ix);
    case 7: return O::A; // "a";
    }

    return {};
}

OperandType Disassembler::regs16_1_ix(u8 p, OperandType ix) const
{
    //static const char* regs[] = { "bc", "de", "??", "sp" };
    static O regs[] = { O::BC, O::DE, O::None, O::SP };
    NX_ASSERT(p >= 0 && p < 4);
    return p == 2 ? ix : regs[p];
}

OperandType Disassembler::regs16_2_ix(u8 p, OperandType ix) const
{
    //static const char* regs[] = { "bc", "de", "??", "af" };
    static O regs[] = { O::BC, O::DE, O::None, O::AF };
    NX_ASSERT(p >= 0 && p < 4);
    return p == 2 ? ix : regs[p];
}

OperandType Disassembler::flags(u8 y) const
{
    //static const char* flags[] = { "nz", "z", "nc", "c", "po", "pe", "p", "m" };
    static O flags[] = { O::NZ, O::Z, O::NC, O::C, O::PO, O::PE, O::P, O::M };
    NX_ASSERT(y >= 0 && y < 8);
    return flags[y];
}

T Disassembler::aluOpCode(u8 y) const
{
    //static const char* aluOpcodes[] = { "add", "adc", "sub", "sbc", "and", "xor", "or", "cp" };
    static T aluOpcodes[] = { T::ADD, T::ADC, T::SUB, T::SBC, T::AND, T::XOR, T::OR, T::CP };
    NX_ASSERT(y >= 0 && y < 8);
    return aluOpcodes[y];
}

bool Disassembler::aluOperandPrefix(u8 y) const
{
    //static const char* prefixes[] = { "a,", "a,", "", "a,", "", "", "", "" };
    static bool prefixes[] = { true, true, false, true, false, false, false, false };
    NX_ASSERT(y >= 0 && y < 8);
    return prefixes[y];
}

T Disassembler::rotShift(u8 y) const
{
    //static const char* opCodes[] = { "rlc", "rrc", "rl", "rr", "sla", "sra", "sl1", "srl" };
    static T opCodes[] = { T::RLC, T::RRC, T::RL, T::RR, T::SLA, T::SRA, T::SL1, T::SRL };
    NX_ASSERT(y >= 0 && y < 8);
    return opCodes[y];
}

u16 Disassembler::disassemble(u16 a, u8 b1, u8 b2, u8 b3, u8 b4)
{
    u8 x, y, z, p, q;
    decode(b1, x, y, z, p, q);
    m_bytes = { b1, b2, b3, b4 };
    m_srcAddr = a;

    switch (x)
    {
    case 0:
        switch (z)
        {
        case 0: // x, z = (0, 0)
            switch (y)
            {
            case 0: result(T::NOP, 1);                                                          break;
            case 1: result(T::EX, O::AF, O::AF_, 1);                                            break;
            case 2: result(T::DJNZ, O::Expression16, displacement(a, b2, 2), 2);                break;
            case 3: result(T::JR, O::Expression16, displacement(a, b2, 2), 2);                  break;
            default: result(T::JR, flags(y - 4), O::Expression16, displacement(a, b2, 2), 2);
            }
            break;

        case 1: // x, z = (0, 1)
            if (q) result(T::ADD, O::HL, regs16_1(p), 1);
            else result(T::LD, regs16_1(p), O::Expression16, b2 + 256 * b3, 3);
            break;

        case 2: // x, z = (0, 2)
            if (0 == q)
            {
                switch (p)
                {
                case 0: result(T::LD, O::Address_BC, O::A, 1);                              break;
                case 1: result(T::LD, O::Address_DE, O::A, 1);                              break;
                case 2: result(T::LD, O::AddressedExpression, word(b2, b3), O::HL, 3);      break;
                case 3: result(T::LD, O::AddressedExpression, word(b2, b3), O::A, 3);       break;
                }
            }
            else
            {
                switch (p)
                {
                case 0: result(T::LD, O::A, O::Address_BC, 1);                              break;
                case 1: result(T::LD, O::A, O::Address_DE, 1);                              break;
                case 2: result(T::LD, O::HL, O::AddressedExpression, word(b2, b3), 3);      break;
                case 3: result(T::LD, O::A, O::AddressedExpression, word(b2, b3), 3);       break;
                }
            }
            break;

        case 3: // x, z = (0, 3)
            result(q ? T::DEC : T::INC, regs16_1(p), 1);
            break;

        case 4: // x, z = (0, 4)
            result(T::INC, regs8(y), 1);
            break;

        case 5: // x, z = (0, 5)
            result(T::DEC, regs8(y), 1);
            break;

        case 6: // x, z = (0, 6)
            result(T::LD, regs8(y), O::Expression8, byte(b2), 2);
            break;

        case 7: // x, z = (0, 7)
            switch (y)
            {
            case 0: result(T::RLCA, 1);     break;
            case 1: result(T::RRCA, 1);     break;
            case 2: result(T::RLA, 1);      break;
            case 3: result(T::RRA, 1);      break;
            case 4: result(T::DAA, 1);      break;
            case 5: result(T::CPL, 1);      break;
            case 6: result(T::SCF, 1);      break;
            case 7: result(T::CCF, 1);      break;
            }
            break;
        }
        break; // x = 0

    case 1:
        if (b1 == 0x76)
        {
            result(T::HALT, 1);
        }
        else
        {
            result(T::LD, regs8(y), regs8(z), 1);
        }
        break; // x = 1

    case 2:
        if (aluOperandPrefix(y))
        {
            result(aluOpCode(y), O::A, regs8(z), 1);
        }
        else
        {
            result(aluOpCode(y), regs8(z), 1);
        }
        break; // x = 2

    case 3:
        switch (z)
        {
        case 0: // x, z = (3, 0)
            result(T::RET, flags(y), 1);
            break;

        case 1: // x, z = (3, 1)
            if (q)
            {
                switch (p)
                {
                case 0: result(T::RET, 1);                  break;
                case 1: result(T::EXX, 1);                  break;
                case 2: result(T::JP, O::HL, 1);            break;
                case 3: result(T::LD, O::SP, O::HL, 1);     break;
                }
            }
            else
            {
                result(T::POP, regs16_2(p), 1);
            }
            break;

        case 2: // x, z = (3, 2)
            result(T::JP, flags(y), O::Expression16, word(b2, b3), 3);
            break;

        case 3: // x, z = (3, 3)
            switch (y)
            {
            case 0: result(T::JP, O::Expression16, word(b2, b3), 3);                break;
            case 1: disassembleCB(b2);                                              break;
            case 2: result(T::OUT, O::AddressedExpression8, byte(b2), O::A, 2);     break;
            case 3: result(T::INC, O::A, O::AddressedExpression8, byte(b2), 2);     break;
            case 4: result(T::EX, O::Address_SP, O::HL, 1);                         break;
            case 5: result(T::EX, O::DE, O::HL, 1);                                 break;
            case 6: result(T::DI, 1);                                               break;
            case 7: result(T::EI, 1);                                               break;
            }
            break;

        case 4: // x, z = (3, 4)
            result(T::CALL, flags(y), O::Expression16, word(b2, b3), 3);
            break;

        case 5: // x, z = (3, 5)
            if (q)
            {
                switch (p)
                {
                case 0: result(T::CALL, O::Expression16, word(b2, b3), 3);          break;
                case 1: disassembleDDFD(b1, b2, b3, b4, O::IX);                     break;
                case 2: disassembleED(b2, b3, b4);                                  break;
                case 3: disassembleDDFD(b1, b2, b3, b4, O::IY);                     break;
                }
            }
            else
            {
                result(T::PUSH, regs16_2(p), 1);
            }
            break;

        case 6: // x, z = (3, 6)
            if (aluOperandPrefix(y))
            {
                result(aluOpCode(y), O::A, O::Expression8, byte(b2), 2);
            }
            else
            {
                result(aluOpCode(y), O::Expression8, byte(b2), 2);
            }
            break;

        case 7: // x, z = (3, 7)
            result(T::RST, O::Expression8, byte(y * 8), 1);
            break;
        }
        break; // x = 3
    } // x

    return a + (u16)m_bytes.size();
}

void Disassembler::disassembleCB(u8 b2)
{
    u8 x, y, z, _;
    decode(b2, x, y, z, _, _);

    switch (x)
    {
    case 0: result(rotShift(y), regs8(z), 2);                       break;
    case 1: result(T::BIT, O::Expression4, i64(y), regs8(z), 2);    break;
    case 2: result(T::RES, O::Expression4, i64(y), regs8(z), 2);    break;
    case 3: result(T::SET, O::Expression4, i64(y), regs8(z), 2);    break;
    }
}

void Disassembler::disassembleDDFD(u8 b1, u8 b2, u8 b3, u8 b4, OperandType ix)
{
    u8 x, y, z, p, q;
    decode(b2, x, y, z, p, q);

    switch (x)
    {
    case 0: // x = 0
        switch (z)
        {
        case 1:
            if (q)
            {
                result(T::ADD, ix, regs16_1_ix(p, ix), 2);
            }
            else
            {
                if (2 == p) result(T::LD, ix, O::Expression16, word(b3, b4), 4);
                else goto invalid;
            }
            break;

        case 2:
            if (2 == p)
            {
                if (q)
                {
                    result(T::LD, ix, O::AddressedExpression, word(b3, b4), 4);
                }
                else
                {
                    result(T::LD, O::AddressedExpression, word(b3, b4), ix, 4);
                }
            }
            else goto invalid;
            break;

        case 3:
            if (2 == p)
            {
                result(q ? T::DEC : T::INC, ix, 2);
            }
            else goto invalid;
            break;

        case 4:
            switch (y)
            {
            case 4: result(T::INC, ixh(ix), 2);                     break;
            case 5: result(T::INC, ixl(ix), 2);                     break;
            case 6: result(T::INC, ixExpr(ix), (i64)b3, 3);         break;
            default: goto invalid;
            }
            break;

        case 5:
            switch (y)
            {
            case 4: result(T::DEC, ixh(ix), 2);                     break;
            case 5: result(T::DEC, ixl(ix), 2);                     break;
            case 6: result(T::DEC, ixExpr(ix), (i64)b3, 3);         break;
            default: goto invalid;
            }
            break;

        case 6:
            switch (y)
            {
            case 4: result(T::LD, ixh(ix), O::Expression8, byte(b3), 3);                    break;
            case 5: result(T::LD, ixl(ix), O::Expression8, byte(b3), 3);                    break;
            case 6: result(T::LD, ixExpr(ix), (i64)b3, O::Expression8, byte(b4), 4);        break;
            default: goto invalid;
            }
            break;

        default:
            goto invalid;
        }
        break;

    case 1: // x = 1
        if (y != 4 && y != 5 && y != 6 && z != 4 && z != 5 && z != 6) goto invalid;
        if (b2 == 0x76) goto invalid;
        if (y == 6 && z != 6)
        {
            // ld (ix+d),r
            result(T::LD, ixExpr(ix), (i64)b3, regs8(z), 3);
        }
        else if (y != 6 && z == 6)
        {
            // ld r,(ix+d)
            result(T::LD, regs8(y), ixExpr(ix), (i64)b3, 3);
        }
        else
        {
            if (y == 6 && z == 6)
            {
                goto invalid;
            }
            else
            {
                int sz = y == 6 || z == 6 ? 3 : 2;
                result(T::LD, regs8_ix(y, ix), (i64)b3, regs8_ix(z, ix), (i64)b3, sz);
            }
        }
        break;

    case 2: // x = 2
        if (z != 4 && z != 5 && z != 6) goto invalid;
        if (int sz = z == 6 ? 3 : 2; aluOperandPrefix(y))
        {
            result(aluOpCode(y), O::A, regs8_ix(z, ix), sz);
        }
        else
        {
            result(aluOpCode(y), regs8_ix(z, ix), sz);
        }
        break;

    case 3: // x = 3
        switch (b2)
        {
        case 0xcb:
            disassembleDDFDCB(b3, b4, ix);
            break;

        case 0xe1:
            result(T::POP, ix, 2);
            break;

        case 0xe3:
            result(T::EX, O::SP, ix, 2);
            break;

        case 0xe5:
            result(T::PUSH, ix, 2);
            break;

        case 0xe9:
            result(T::JP, ix, 2);
            break;

        case 0xf9:
            result(T::LD, O::Address_SP, ix, 2);
            break;

        default:
            goto invalid;
        }
        break;
    }

    return;

invalid:
    result(T::DB, O::Expression8, byte(b1), 1);
}

void Disassembler::disassembleDDFDCB(u8 b3, u8 b4, OperandType ix)
{
    u8 x, y, z, _;
    decode(b4, x, y, z, _, _);

    switch (x)
    {
    case 0:
        if (z == 6)
        {
            // rot/shift[y] (IX+d)
            result(rotShift(y), O::IX_Expression, (i64)b3, 4);
        }
        else
        {
            // LD R[z],rot/shift[y] (IX+d)
            result(T::LD, rotShift(y), regs8(z), 0, ixExpr(ix), i64(b3), 4);
        }
        break;

    case 1:
        result(T::BIT, O::Expression4, y, ixExpr(ix), (i64)b3, 4);
        break;

    case 2:
        if (z == 6)
        {
            // RES y,(IX+d)
            result(T::RES, O::Expression4, y, ixExpr(ix), (i64)b3, 4);
        }
        else
        {
            // LD R[z],RES y,(IX+d)
            result(T::LD, T::RES, regs8(z), i64(y), ixExpr(ix), i64(b3), 4);
        }
        break;

    case 3:
        if (z == 6)
        {
            // SET y,(IX+d)
            result(T::SET, O::Expression4, y, ixExpr(ix), (i64)b3, 4);
        }
        else
        {
            // LD R[z],SET y,(IX+d)
            result(T::LD, T::SET, regs8(z), i64(y), ixExpr(ix), i64(b3), 4);
        }
        break;
    }
}

void Disassembler::disassembleED(u8 b2, u8 b3, u8 b4)
{
    u8 x, y, z, p, q;
    decode(b2, x, y, z, p, q);

    switch (x)
    {
    case 0:
    case 3:
        goto invalid;

    case 1:
        switch (z)
        {
        case 0: result(T::IN, (y == 6 ? O::F : regs8(y)), O::Address_C, 2);                 break;
        case 1: result(T::OUT, O::Address_C, (y == 6 ? O::Expression8 : regs8(y)), 0, 2);   break;
        case 2: result(q ? T::ADC : T::SBC, O::HL, regs16_1(p), 2);                         break;
        case 3:
            if (q)
            {
                result(T::LD, regs16_1(p), O::AddressedExpression, word(b3, b4), 4);
            }
            else
            {
                result(T::LD, O::AddressedExpression, word(b3, b4), regs16_1(p), 4);
            }
            break;
        case 4: result(T::NEG, 2);                                                          break;
        case 5: result(y == 1 ? T::RETI : T::RETN, 2);                                      break;
        case 6: result(T::IM, O::Expression4, (y & 3) == 0 ? 0 : i64(y - 1), 2);            break;
        case 7:
            switch (y)
            {
            case 0: result(T::LD, O::I, O::A, 2);   break;
            case 1: result(T::LD, O::R, O::A, 2);   break;
            case 2: result(T::LD, O::A, O::I, 2);   break;
            case 3: result(T::LD, O::A, O::R, 2);   break;
            case 4: result(T::RRD, 2);              break;
            case 5: result(T::RLD, 2);              break;
            case 6: result(T::NOP, 2);              break;
            case 7: result(T::NOP, 2);              break;
            }
            break;
        }
        break;

    case 2:
    {
        static T ops[] = {
            T::LDI,     T::CPI,     T::INI,     T::OUTI,
            T::LDD,     T::CPD,     T::IND,     T::OUTD,
            T::LDIR,    T::CPIR,    T::INIR,    T::OTIR,
            T::LDDR,    T::CPDR,    T::INDR,    T::OTDR,
        };
        if (z <= 3 && y >= 4)
        {
            result(ops[(y - 4) * 4 + z], 2);
        }
        else goto invalid;
    }
    break;
    }

    return;

invalid:
    result(T::DB, O::Expression8, (i64)0xed, 1);
}

std::string Disassembler::addressAndBytes(u16 a) const
{
    std::string s = wordNoPrefix(a % 256, a / 256) + "  ";
    for (int i = 0; i < m_bytes.size(); ++i)
    {
        s += byteNoPrefix(m_bytes[i]) + " ";
    }

    return s;
}

std::string Disassembler::opCodeString(T type)
{
    static const char* opCodeStrings[(int)T::_END_DIRECTIVES - (int)T::_KEYWORDS - 1] =
    {
        "adc",
        "add",
        "and",
        "bit",
        "call",
        "ccf",
        "cp",
        "cpd",
        "cpdr",
        "cpi",
        "cpir",
        "cpl",
        "daa",
        "dec",
        "di",
        "djnz",
        "ei",
        "ex",
        "exx",
        "halt",
        "im",
        "in",
        "inc",
        "ind",
        "indr",
        "ini",
        "inir",
        "jp",
        "jr",
        "ld",
        "ldd",
        "lddr",
        "ldi",
        "ldir",
        "neg",
        "nop",
        "or",
        "otdr",
        "otir",
        "out",
        "outd",
        "outi",
        "pop",
        "push",
        "res",
        "ret",
        "reti",
        "retn",
        "rl",
        "rla",
        "rlc",
        "rlca",
        "rld",
        "rr",
        "rra",
        "rrc",
        "rrca",
        "rrd",
        "rst",
        "sbc",
        "scf",
        "set",
        "sla",
        "sll",
        "sl1",
        "sra",
        "srl",
        "sub",
        "xor",
        "???",
        "db",
        "dw",
        "equ",
        "load",
        "opt",
        "org",
    };
    return opCodeStrings[(int)type - (int)T::_KEYWORDS - 1];
}

// opCode2 is normally T::Unknown.  If not, param2 is the 1st operand value.
//
std::string Disassembler::operandString(Operand op, Lex::Element::Type opCode2, i64 param2,
    const Spectrum& speccy, const Addresses& addresses)
{
    if (opCode2 == T::Unknown)
    {
        switch (op.type)
        {
        case O::Expression:
        case O::Expression4:
            return intString((int)op.param, 0);

        case O::AddressedExpression:
            {
                MemAddr a = speccy.convertAddress(Z80MemAddr((u16)op.param));
                if (a.bank().getGroup() == MemGroup::RAM)
                {
                    auto it = addresses.find(a);
                    if (it == addresses.end())
                    {
                        return string("($") + hexWord(u16(op.param)) + ')';
                    }
                    else
                    {
                        return string("(") + it->second.first + ')';
                    }
                }
                else
                {
                    return string("($") + hexWord(u16(op.param)) + ')';
                }
            }

        case O::IX_Expression:
            return string("(ix") + ((i8)(u8)op.param < 0 ? "" : "+") + intString((int)(i8)(u8)op.param, 0) + ')';

        case O::IY_Expression:
            return string("(iy") + ((i8)(u8)op.param < 0 ? "" : "+") + intString((int)(i8)(u8)op.param, 0) + ')';

        case O::A:	                    return "a";
        case O::B:	                    return "b";
        case O::C:	                    return "c";
        case O::D:	                    return "d";
        case O::E:	                    return "e";
        case O::H:	                    return "h";
        case O::L:	                    return "l";
        case O::I:	                    return "i";
        case O::R:	                    return "r";
        case O::AF:	                    return "af";
        case O::AF_:	                return "af'";
        case O::BC:	                    return "bc";
        case O::DE:	                    return "de";
        case O::HL:	                    return "hl";
        case O::IX:	                    return "ix";
        case O::IY:	                    return "iy";
        case O::IXH:	                return "ixh";
        case O::IXL:	                return "ixl";
        case O::IYH:	                return "iyh";
        case O::IYL:	                return "iyl";
        case O::SP:	                    return "sp";
        case O::NC:	                    return "nc";
        case O::Z:	                    return "z";
        case O::NZ:	                    return "nz";
        case O::PO:	                    return "po";
        case O::PE:	                    return "pe";
        case O::M:	                    return "m";
        case O::P:	                    return "p";

        case O::Address_BC:             return "(bc)";
        case O::Address_DE:             return "(de)";
        case O::Address_HL:             return "(hl)";
        case O::Address_SP:             return "(sp)";
        case O::Address_C:              return "(c)";

        case O::Expression8:            return string("$") + hexByte(u8(op.param));
        case O::Expression16:
            {
                MemAddr a = speccy.convertAddress(Z80MemAddr((u16)op.param));
                if (a.bank().getGroup() == MemGroup::RAM)
                {
                    auto it = addresses.find(a);
                    if (it == addresses.end())
                    {
                        return string("$") + hexWord(u16(op.param));
                    }
                    else
                    {
                        return it->second.first;
                    }
                }
                else
                {
                    return string("$") + hexWord(u16(op.param));
                }
            }

        case O::AddressedExpression8:   return string("($") + hexByte(u8(op.param)) + ')';

        case O::F:	                    return "f";

        default:
            NX_ASSERT(0);
            return "???";
        }
    }
    else
    {
        if (opCode2 == T::RES || opCode2 == T::SET)
        {
            // <OPCODE> n,<OPERAND STRING>
            return opCodeString(opCode2) + " " + intString((int)param2, 0) + "," + operandString(op, T::Unknown, 0, speccy, addresses);
        }
        else
        {
            // <OPCODE> <OPERAND STRING>
            return opCodeString(opCode2) + " " + operandString(op, T::Unknown, 0, speccy, addresses);
        }
    }
}

std::string Disassembler::opCodeString() const
{
    return opCodeString(m_opCode);
}

std::string Disassembler::operand1String(const Spectrum& speccy, const Addresses& addresses) const
{
    return operandString(m_operands[0], T::Unknown, 0, speccy, addresses);
}

std::string Disassembler::operand2String(const Spectrum& speccy, const Addresses& addresses) const
{
    return operandString(m_operands[1], m_opCode2, m_operands[0].param, speccy, addresses);
}

std::string Disassembler::operandString(const Spectrum& speccy, const Addresses& addresses) const
{
    if (operand1Value() == O::None)
    {
        return "";
    }
    else
    {
        string s = operand1String(speccy, addresses);
        if (operand2Value() == O::None)
        {
            return s;
        }
        else
        {
            return s + "," + operand2String(speccy, addresses);
        }
    }
}

void Disassembler::result(Lex::Element::Type opCode, int instructionSize)
{
    result(opCode, T::Unknown, O::None, 0, O::None, 0, instructionSize);
}

void Disassembler::result(Lex::Element::Type opCode, OperandType op1, int instructionSize)
{
    result(opCode, T::Unknown, op1, 0, O::None, 0, instructionSize);
}

void Disassembler::result(Lex::Element::Type opCode, OperandType op1, OperandType op2, int instructionSize)
{
    result(opCode, T::Unknown, op1, 0, op2, 0, instructionSize);
}

void Disassembler::result(Lex::Element::Type opCode, OperandType op1, i64 value1, int instructionSize)
{
    result(opCode, T::Unknown, op1, value1, O::None, 0, instructionSize);
}

void Disassembler::result(Lex::Element::Type opCode, OperandType op1, i64 value1, OperandType op2, int instructionSize)
{
    result(opCode, T::Unknown, op1, value1, op2, 0, instructionSize);
}

void Disassembler::result(Lex::Element::Type opCode, OperandType op1, OperandType op2, i64 value2, int instructionSize)
{
    result(opCode, T::Unknown, op1, 0, op2, value2, instructionSize);
}

void Disassembler::result(Lex::Element::Type opCode, OperandType op1, i64 value1, OperandType op2, i64 value2, int instructionSize)
{
    result(opCode, T::Unknown, op1, value1, op2, value2, instructionSize);
}

void Disassembler::result(Lex::Element::Type opCode, Lex::Element::Type opCode2, OperandType op1, i64 value1, OperandType op2, i64 value2, int instructionSize)
{
    m_opCode = opCode;
    m_opCode2 = opCode2;
    m_operands[0].type = op1;
    m_operands[0].param = value1;
    m_operands[1].type = op2;
    m_operands[1].param = value2;
    m_bytes.erase(m_bytes.begin() + instructionSize, m_bytes.end());
}

optional<u16> Disassembler::extractAddress() const
{
    static const int addresses[256] = {
    //  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
        0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 00-0F
        5,  1,  0,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,      // 10-1F
        5,  1,  1,  0,  0,  0,  0,  0,  5,  0,  1,  0,  0,  0,  0,  0,      // 20-2F
        5,  1,  1,  0,  0,  0,  0,  0,  5,  0,  1,  0,  0,  0,  0,  0,      // 30-3F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 40-4F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 50-5F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 60-6F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 70-7F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 80-8F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 90-9F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // A0-AF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // B0-BF
        0,  0,  1,  1,  1,  0,  0,  0,  0,  0,  1,  0,  1,  1,  0,  0,      // C0-CF
        0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,      // D0-DF
        0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,      // E0-EF
        0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,      // F0-FF
    };

    static const int ixAddresses[256] = {
    //  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 00-0F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 10-1F
        0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,      // 20-2F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 30-3F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 40-4F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 50-5F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 60-6F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 70-7F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 80-8F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 90-9F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // A0-AF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // B0-BF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // C0-CF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // D0-DF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // E0-EF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // F0-FF
    };

    static const int edAddresses[256] = {
    //  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 00-0F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 10-1F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 20-2F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 30-3F
        0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,      // 40-4F
        0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,      // 50-5F
        0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,      // 60-6F
        0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,      // 70-7F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 80-8F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // 90-9F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // A0-AF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // B0-BF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // C0-CF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // D0-DF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // E0-EF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,      // F0-FF
    };

    const int* adds = addresses;

    int i = 0;
    switch (m_bytes[0])
    {
    case 0xdd:
    case 0xfd:
        adds = ixAddresses;
        ++i;
        break;

    case 0xed:
        adds = edAddresses;
        ++i;
        break;
    }

    int offset = adds[m_bytes[0]];
    u16 a = 0;
    if (offset == 0) return {};
    if (offset >= 4)
    {
        offset -= 4;
        a = m_srcAddr + (u16)m_bytes.size() + (u16)m_bytes[i + offset];

    }
    else
    {
        a = (u16)m_bytes[i + offset] + (u16)m_bytes[i + offset + 1] * 256;
    }

    return a;

}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

