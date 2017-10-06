//----------------------------------------------------------------------------------------------------------------------
// Z80 Disassembler
//----------------------------------------------------------------------------------------------------------------------

#pragma once

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
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <cassert>
#include <sstream>
#include <iomanip>

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

void Disassembler::result(std::string&& opCode, std::string&& operands, int instructionSize)
{
    m_opCode = std::move(opCode);
    m_operands = std::move(operands);
    m_comment = std::string();

    for (int i = instructionSize; i < 4; ++i) m_bytes.pop_back();
}

void Disassembler::result(std::string&& opCode, int instructionSize)
{
    result(std::move(opCode), std::string(), instructionSize);
}

void Disassembler::invalidOpCode()
{
    result("defb", byte(m_bytes[0]), 1);
}

std::string Disassembler::word(u8 l, u8 h)
{
    std::stringstream ss;
    ss << '$'
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)h
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)l;
    return ss.str();
}

std::string Disassembler::byte(u8 b)
{
    std::stringstream ss;
    ss << '$' << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)b;
    return ss.str();
}

std::string Disassembler::wordNoPrefix(u8 l, u8 h)
{
    std::stringstream ss;
    ss
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)h
        << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)l;
    return ss.str();
}

std::string Disassembler::byteNoPrefix(u8 b)
{
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)b;
    return ss.str();
}

std::string Disassembler::index(u8 b)
{
    std::stringstream ss;
    ss << (int)b;
    return ss.str();
}

std::string Disassembler::displacement(u16 a, u8 d, int instructionSize)
{
    a += instructionSize + (int)(i8)d;
    return word(a % 256, a / 256);
}

std::string Disassembler::indexDisplacement(u8 d, std::string ix)
{
    i8 x = (i8)d;
    return "(" + ix + (x < 0 ? "-" : "+") + byte(x < 0 ? -d : d) + ")";
}

std::string Disassembler::regs8(u8 y)
{
    static const char* regs[] = { "b", "c", "d", "e", "h", "l", "(hl)", "a" };
    assert(y >= 0 && y < 8);
    return regs[y];
}

std::string Disassembler::regs16_1(u8 p)
{
    static const char* regs[] = { "bc", "de", "hl", "sp" };
    assert(p >= 0 && p < 4);
    return regs[p];
}

std::string Disassembler::regs16_2(u8 p)
{
    static const char* regs[] = { "bc", "de", "hl", "af" };
    assert(p >= 0 && p < 4);
    return regs[p];
}

std::string Disassembler::regs8_ix(u8 y, std::string ix, u8 d)
{
    assert(y >= 0 && y < 8);
    switch (y)
    {
    case 0: return "b";
    case 1: return "c";
    case 2: return "d";
    case 3: return "e";
    case 4: return ix + "h";
    case 5: return ix + "l";
    case 6: return indexDisplacement(d, ix);
    case 7: return "a";
    }

    return {};
}

std::string Disassembler::regs16_1_ix(u8 p, std::string ix)
{
    static const char* regs[] = { "bc", "de", "??", "sp" };
    assert(p >= 0 && p < 4);
    return p == 2 ? ix : regs[p];
}

std::string Disassembler::regs16_2_ix(u8 p, std::string ix)
{
    static const char* regs[] = { "bc", "de", "??", "af" };
    assert(p >= 0 && p < 4);
    return p == 2 ? ix : regs[p];
}

std::string Disassembler::flags(u8 y)
{
    static const char* flags[] = { "nz", "z", "nc", "c", "po", "pe", "p", "m" };
    assert(y >= 0 && y < 8);
    return flags[y];
}

std::string Disassembler::aluOpCode(u8 y)
{
    static const char* aluOpcodes[] = { "add", "adc", "sub", "sbc", "and", "xor", "or", "cp" };
    assert(y >= 0 && y < 8);
    return aluOpcodes[y];
}

std::string Disassembler::aluOperandPrefix(u8 y)
{
    static const char* prefixes[] = { "a,", "a,", "", "a,", "", "", "", "" };
    assert(y >= 0 && y < 8);
    return prefixes[y];
}

std::string Disassembler::rotShift(u8 y)
{
    static const char* opCodes[] = { "rlc", "rrc", "rl", "rr", "sla", "sra", "sl1", "srl" };
    assert(y >= 0 && y < 8);
    return opCodes[y];
}

u16 Disassembler::disassemble(u16 a, u8 b1, u8 b2, u8 b3, u8 b4)
{
    u8 x, y, z, p, q;
    decode(b1, x, y, z, p, q);
    m_bytes = { b1, b2, b3, b4 };

    switch (x)
    {
    case 0:
        switch (z)
        {
        case 0: // x, z = (0, 0)
            switch (y)
            {
            case 0: result("nop", 1);                                               break;
            case 1: result("ex", "af,af'", 1);                                      break;
            case 2: result("djnz", displacement(a, b2, 2), 2);                         break;
            case 3: result("jr", displacement(a, b2, 2), 2);                           break;
            default: result("jr", flags(y - 4) + "," + displacement(a, b2, 2), 2);
            }
            break;

        case 1: // x, z = (0, 1)
            if (q) result("add", "hl," + regs16_1(p), 1);
            else result("ld", regs16_1(p) + "," + word(b2, b3), 3);
            break;

        case 2: // x, z = (0, 2)
            if (0 == q)
            {
                switch (p)
                {
                case 0: result("ld", "(bc),a", 1);                      break;
                case 1: result("ld", "(de),a", 1);                      break;
                case 2: result("ld", "(" + word(b2, b3) + "),hl", 3);   break;
                case 3: result("ld", "(" + word(b2, b3) + "),a", 3);    break;
                }
            }
            else
            {
                switch (p)
                {
                case 0: result("ld", "a,(bc)", 1);                      break;
                case 1: result("ld", "a,(de)", 1);                      break;
                case 2: result("ld", "hl,(" + word(b2, b3) + ")", 3);   break;
                case 3: result("ld", "a,(" + word(b2, b3) + ")", 3);    break;
                }
            }
            break;

        case 3: // x, z = (0, 3)
            result(q ? "dec" : "inc", regs16_1(p), 1);
            break;

        case 4: // x, z = (0, 4)
            result("inc", regs8(y), 1);
            break;

        case 5: // x, z = (0, 5)
            result("dec", regs8(y), 1);
            break;

        case 6: // x, z = (0, 6)
            result("ld", regs8(y) + "," + byte(b2), 2);
            break;

        case 7: // x, z = (0, 7)
            switch (y)
            {
            case 0: result("rlca", 1);  break;
            case 1: result("rrca", 1);  break;
            case 2: result("rla", 1);   break;
            case 3: result("rra", 1);   break;
            case 4: result("daa", 1);   break;
            case 5: result("cpl", 1);   break;
            case 6: result("scf", 1);   break;
            case 7: result("ccf", 1);   break;
            }
            break;
        }
        break; // x = 0

    case 1:
        if (b1 == 0x76)
        {
            result("halt", 1);
        }
        else
        {
            result("ld", regs8(y) + "," + regs8(z), 1);
        }
        break; // x = 1

    case 2:
        result(aluOpCode(y), aluOperandPrefix(y) + regs8(z), 1);
        break; // x = 2

    case 3:
        switch (z)
        {
        case 0: // x, z = (3, 0)
            result("ret", flags(y), 1);
            break;

        case 1: // x, z = (3, 1)
            if (q)
            {
                switch (p)
                {
                case 0: result("ret", 1);           break;
                case 1: result("exx", 1);           break;
                case 2: result("jp", "hl", 1);      break;
                case 3: result("ld" "sp,hl", 1);    break;
                }
            }
            else
            {
                result("pop", regs16_2(p), 1);
            }
            break;

        case 2: // x, z = (3, 2)
            result("jp", flags(y) + word(b2, b3), 3);
            break;

        case 3: // x, z = (3, 3)
            switch (y)
            {
            case 0: result("jp", word(b2, b3), 3);              break;
            case 1: disassembleCB(b2);                          break;
            case 2: result("out", "(" + byte(b2) + "),a", 2);   break;
            case 3: result("in", "a,(" + byte(b2) + ")", 2);    break;
            case 4: result("ex", "(sp),hl", 1);                 break;
            case 5: result("ex", "de,hl", 1);                   break;
            case 6: result("di", 1);                            break;
            case 7: result("ei", 1);                            break;
            }
            break;

        case 4: // x, z = (3, 4)
            result("call", flags(y) + "," + word(b2, b3), 3);
            break;

        case 5: // x, z = (3, 5)
            if (q)
            {
                switch (p)
                {
                case 0: result("call", word(b2, b3), 3);        break;
                case 1: disassembleDDFD(b1, b2, b3, b4, "ix");  break;
                case 2: disassembleED(b2, b3, b4);              break;
                case 3: disassembleDDFD(b1, b2, b3, b4, "iy");  break;
                }
            }
            else
            {
                result("push", regs16_2(p), 1);
            }
            break;

        case 6: // x, z = (3, 6)
            result(aluOpCode(y), aluOperandPrefix(y) + byte(b2), 2);
            break;

        case 7: // x, z = (3, 7)
            result("rst", byte(y * 8), 1);
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
    case 0: result(rotShift(y), regs8(z), 2);               break;
    case 1: result("bit", index(y) + "," + regs8(z), 2);    break;
    case 2: result("res", index(y) + "," + regs8(z), 2);    break;
    case 3: result("set", index(y) + "," + regs8(z), 2);    break;
    }
}

void Disassembler::disassembleDDFD(u8 b1, u8 b2, u8 b3, u8 b4, std::string ix)
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
                result("add", ix + "," + regs16_1_ix(p, ix), 2);
            }
            else
            {
                if (2 == p) result("ld", ix + "," + word(b3, b4), 4);
                else goto invalid;
            }
            break;

        case 2:
            if (2 == p)
            {
                if (q)
                {
                    result("ld", ix + ",(" + word(b3, b4) + ")", 4);
                }
                else
                {
                    result("ld", "(" + word(b3, b4) + ")," + ix, 4);
                }
            }
            else goto invalid;
            break;

        case 3:
            if (2 == p)
            {
                result(q ? "dec" : "inc", std::move(ix), 2);
            }
            else goto invalid;
            break;

        case 4:
            switch (y)
            {
            case 4: result("inc", ix + "h", 2);                     break;
            case 5: result("inc", ix + "l", 2);                     break;
            case 6: result("inc", indexDisplacement(b3, ix), 3);    break;
            default: goto invalid;
            }
            break;

        case 5:
            switch (y)
            {
            case 4: result("dec", ix + "h", 2);                     break;
            case 5: result("dec", ix + "l", 2);                     break;
            case 6: result("dec", indexDisplacement(b3, ix), 3);    break;
            default: goto invalid;
            }
            break;

        case 6:
            switch (y)
            {
            case 4: result("ld", ix + "h" + "," + byte(b3), 3);                     break;
            case 5: result("ld", ix + "l" + "," + byte(b3), 3);                     break;
            case 6: result("ld", indexDisplacement(b3, ix) + "," + byte(b4), 4);    break;
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
            result("ld", indexDisplacement(b3, ix) + "," + regs8(z), 3);
        }
        else if (y != 6 && z == 6)
        {
            // ld r,(ix+d)
            result("ld", regs8(y) + "," + indexDisplacement(b3, ix), 3);
        }
        else
        {
            result("ld", regs8_ix(y, ix, b3) + "," + regs8_ix(z, ix, b3), 3);
        }
        break;

    case 2: // x = 2
        if (z != 4 && z != 5 && z != 6) goto invalid;
        result(aluOpCode(y), aluOperandPrefix(y) + regs8_ix(z, ix, b3), z == 6 ? 3 : 2);
        break;

    case 3: // x = 3
        switch (b2)
        {
        case 0xcb:
            disassembleDDFDCB(b3, b4, ix);
            break;

        case 0xe1:
            result("pop", std::move(ix), 2);
            break;

        case 0xe3:
            result("ex", "(sp)," + ix, 2);
            break;

        case 0xe5:
            result("push", std::move(ix), 2);
            break;

        case 0xe9:
            result("jp", std::move(ix), 2);
            break;

        case 0xf9:
            result("ld", "(sp)," + ix, 2);
            break;

        default:
            goto invalid;
        }
        break;
    }

    return;

invalid:
    result("defb", byte(b1), 1);
}

void Disassembler::disassembleDDFDCB(u8 b3, u8 b4, std::string ix)
{
    u8 x, y, z, _;
    decode(b4, x, y, z, _, _);

    switch (x)
    {
    case 0:
        if (z == 6)
        {
            // rot/shift[y] (IX+d)
            result(rotShift(y), indexDisplacement(b3, ix), 4);
        }
        else
        {
            // LD R[z],rot/shift[y] (IX+d)
            result("ld", regs8(z) + "," + rotShift(y) + " " + indexDisplacement(b3, ix), 4);
        }
        break;

    case 1:
        result("bit", index(y) + "," + indexDisplacement(b3, ix), 4);
        break;

    case 2:
        if (z == 6)
        {
            // RES y,(IX+d)
            result("res", index(y) + "," + indexDisplacement(b3, ix), 4);
        }
        else
        {
            // LD R[z],RES y,(IX+d)
            result("ld", regs8(z) + ",res " + index(y) + "," + indexDisplacement(b3, ix), 4);
        }
        break;

    case 3:
        if (z == 6)
        {
            // SET y,(IX+d)
            result("set", index(y) + "," + indexDisplacement(b3, ix), 4);
        }
        else
        {
            // LD R[z],SET y,(IX+d)
            result("ld", regs8(z) + ",set " + index(y) + "," + indexDisplacement(b3, ix), 4);
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
        case 0: result("in", (y == 6 ? "f" : regs8(y)) + ",(c)", 2);            break;
        case 1: result("out", "(c)," + (y == 6 ? "0" : regs8(y)), 2);           break;
        case 2: result(q ? "adc" : "sbc", "hl," + regs16_1(p), 2);              break;
        case 3: result("ld", q
                            ? (regs16_1(p) + ",(" + word(b3, b4) + ")")
                            : ("(" + word(b3, b4) + ")," + regs16_1(p)), 4);    break;
        case 4: result("neg", 2);                                               break;
        case 5: result(y == 1 ? "reti" : "retn", 2);                            break;
        case 6: result("im", (y & 3) == 0 ? "0" : index(y - 1), 2);             break;
        case 7:
            switch (y)
            {
            case 0: result("ld", "i,a", 2);     break;
            case 1: result("ld", "r,a", 2);     break;
            case 2: result("ld", "a,i", 2);     break;
            case 3: result("ld", "a,r", 2);     break;
            case 4: result("rrd", 2);           break;
            case 5: result("rld", 2);           break;
            case 6: result("nop", 2);           break;
            case 7: result("nop", 2);           break;
            }
            break;
        }
        break;

    case 2:
        {
            static const char* ySuffixes[4] = { "i", "d", "ir", "dr" };
            static const char* zPrefixes1[4] = { "ld", "cp", "in", "out" };
            static const char* zPrefixes2[4] = { "ld", "cp", "in", "ot" };
            if (z <= 3 && y >= 4)
            {
                result(
                       std::string(z > 1 ? zPrefixes2[z] : zPrefixes1[z]) +
                    ySuffixes[y - 4],
                    2);
            }
            else goto invalid;
        }
        break;
    }

    return;

invalid:
    result("defb", "$ed", 1);
}

std::string Disassembler::addressAndBytes(u16 a)
{
    std::string s = wordNoPrefix(a % 256, a / 256) + "  ";
    for (int i = 0; i < m_bytes.size(); ++i)
    {
        s += byteNoPrefix(m_bytes[i]) + " ";
    }

    return s;
}

std::string Disassembler::opCode()
{
    return m_opCode;
}

std::string Disassembler::operands()
{
    return m_operands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
