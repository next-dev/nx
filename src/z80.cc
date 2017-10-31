//----------------------------------------------------------------------------------------------------------------------
// Z80 implementation
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// Debug settings
//----------------------------------------------------------------------------------------------------------------------

#define NX_DEBUG_IN     0
#define NX_DEBUG_OUT    0

//----------------------------------------------------------------------------------------------------------------------

#if NX_DEBUG_IN
#   define NX_LOG_IN(p,b) NX_LOG("In: (%04x) -> %02x\n", p, b);
#else
#   define NX_LOG_IN(...)
#endif

#if NX_DEBUG_OUT
#   define NX_LOG_OUT(p,b) NX_LOG("Out: (%04x) <- %02x\n", p, b);
#else
#   define NX_LOG_OUT(...)
#endif


#include "z80.h"

#include <algorithm>
#include <cassert>

const u8 Z80::kIoIncParityTable[16] = { 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 };
const u8 Z80::kIoDecParityTable[16] = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1 };
const u8 Z80::kHalfCarryAdd[8] = { 0, F_HALF, F_HALF, F_HALF, 0, 0, 0, F_HALF };
const u8 Z80::kHalfCarrySub[8] = { 0, 0, F_HALF, 0, F_HALF, 0, F_HALF, F_HALF };
const u8 Z80::kOverflowAdd[8] = { 0, 0, 0, F_PARITY, F_PARITY, 0, 0, 0 };
const u8 Z80::kOverflowSub[8] = { 0, F_PARITY, 0, 0, 0, 0, F_PARITY, 0 };

//----------------------------------------------------------------------------------------------------------------------
// Flag manipulation
//----------------------------------------------------------------------------------------------------------------------

void Z80::setFlags(u8 flags, bool value)
{
    if (value)
    {
        F() |= flags;
    }
    else
    {
        F() &= ~flags;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Initialisation
//----------------------------------------------------------------------------------------------------------------------

Z80::Z80(IExternals& ext)
    : m_ext(ext)
    , m_halt(false)
    , m_iff1(true)
    , m_iff2(true)
    , m_im(0)
    , m_interrupt(false)
    , m_nmi(false)
    , m_eiHappened(false)
{
    restart();
    for (int i = 0; i < 256; ++i)
    {
        m_SZ53[i] = (u8)(i & (F_3 | F_5 | F_SIGN));

        u8 p = 0, x = i;
        for (int bit = 0; bit < 8; ++bit) { p ^= (u8)(x & 1); x >>= 1; }
        m_parity[i] = p ? 0 : F_PARITY;

        m_SZ53P[i] = m_SZ53[i] | m_parity[i];
    }

    m_SZ53[0] |= F_ZERO;
    m_SZ53P[0] |= F_ZERO;
}

void Z80::restart()
{
    AF() = 0xffff;
    BC() = 0xffff;
    DE() = 0xffff;
    HL() = 0xffff;
    SP() = 0xffff;
    PC() = 0x0000;
    IX() = 0xffff;
    IY() = 0xffff;
    IR() = 0x0000;
    AF_() = 0xffff;
    BC_() = 0xffff;
    DE_() = 0xffff;
    HL_() = 0xffff;
    MP() = 0x0000;
    m_halt = false;
    m_iff1 = m_iff2 = true;
    m_im = 0;
    m_interrupt = m_nmi = m_eiHappened = false;
}

//----------------------------------------------------------------------------------------------------------------------
// Instruction utilities
//----------------------------------------------------------------------------------------------------------------------

void Z80::exx()
{
    std::swap(BC(), BC_());
    std::swap(DE(), DE_());
    std::swap(HL(), HL_());
}

void Z80::exAfAf()
{
    std::swap(AF(), AF_());
}

void Z80::incReg8(u8& reg)
{
    ++reg;

    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 3
    // P: Result is 0x80
    // N: Reset
    // C: Unaffected
    F() = (F() & F_CARRY) | ((reg == 0x80) ? F_PARITY : 0) | ((reg & 0x0f) ? 0 : F_HALF) | m_SZ53[reg];
}

void Z80::decReg8(u8& reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 4
    // P: Result is 0x7f
    // N: Set
    // C: Unaffected
    F() = (F() & F_CARRY) | ((reg & 0x0f) ? 0 : F_HALF) | F_NEG;
    --reg;
    F() |= (reg == 0x7f ? F_PARITY : 0) | m_SZ53[reg];
}

void Z80::addReg16(u16& r1, u16& r2)
{
    u32 add = (u32)r1 + (u32)r2;

    // S: Not affected
    // Z: Not affected
    // H: Set if carry from bit 11
    // P: Not affected
    // N: Reset
    // C: Carry from bit 15
    u8 x = (u8)(((r1 & 0x0800) >> 11) | ((r2 & 0x0800) >> 10) | ((add & 0x0800) >> 9));
    F() = (F() & (F_PARITY | F_ZERO | F_SIGN)) | (add & 0x10000 ? F_CARRY : 0) | ((add >> 8) & (F_3 | F_5)) | kHalfCarryAdd[x];
    r1 = (u16)add;
}

// Result always goes into A.
void Z80::addReg8(u8& reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 3
    // P: Set if overflow
    // N: Reset
    // C: Carry from bit 7
    u16 t = A() + reg;
    u8 x = (u8)(((A() & 0x88) >> 3) | ((reg & 0x88) >> 2) | ((t & 0x88) >> 1));
    A() = (u8)t;
    F() = ((t & 0x100) ? F_CARRY : 0) | kHalfCarryAdd[x & 0x07] | kOverflowAdd[x >> 4] | m_SZ53[A()];
}

// Result always goes into HL
void Z80::adcReg16(u16& reg)
{
    // S: Not affected
    // Z: Not affected
    // H: Set if carry from bit 11
    // P: Not affected
    // N: Reset
    // C: Carry from bit 15
    u32 t = (u32)HL() + (u32)reg + (F() & F_CARRY);
    u8 x = (u8)(((HL() & 0x8800) >> 11) | ((reg & 0x8800) >> 10) | ((t & 0x8800) >> 9));
    MP() = HL() + 1;
    HL() = (u16)t;
    F() = ((t & 0x10000) ? F_CARRY : 0) | kOverflowAdd[x >> 4] | (H() & (F_3 | F_5 | F_SIGN)) | kHalfCarryAdd[x & 0x07] |
        (HL() ? 0 : F_ZERO);
}

// Result always goes into A
void Z80::adcReg8(u8& reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 3
    // P: Set if overflow
    // N: Reset
    // C: Carry from bit 7
    u16 t = (u16)A() + reg + (F() & F_CARRY);
    u8 x = (u8)(((A() & 0x88) >> 3) | ((reg & 0x88) >> 2) | ((t & 0x88) >> 1));
    A() = (u8)t;
    F() = ((t & 0x100) ? F_CARRY : 0) | kHalfCarryAdd[x & 0x07] | kOverflowAdd[x >> 4] | m_SZ53[A()];
}

void Z80::subReg8(u8& reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Borrow from bit 4
    // P: Set if overflow
    // N: Set
    // C: Set if borrowed
    u16 t = (u16)A() - reg;
    u8 x = (u8)(((A() & 0x88) >> 3) | ((reg & 0x88) >> 2) | ((t & 0x88) >> 1));
    A() = (u8)t;
    F() = ((t & 0x100) ? F_CARRY : 0) | F_NEG | kHalfCarrySub[x & 0x07] | kOverflowSub[x >> 4] | m_SZ53[A()];
}

void Z80::sbcReg8(u8& reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Borrow from bit 4
    // P: Set if overflow
    // N: Set
    // C: Set if borrowed
    u16 t = (u16)A() - reg - (F() & F_CARRY);
    u8 x = (u8)(((A() & 0x88) >> 3) | ((reg & 0x88) >> 2) | ((t & 0x88) >> 1));
    A() = (u8)t;
    F() = ((t & 0x100) ? F_CARRY : 0) | F_NEG | kHalfCarrySub[x & 0x07] | kOverflowSub[x >> 4] | m_SZ53[A()];
}

void Z80::sbcReg16(u16& reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Borrow from bit 12
    // P: Set if overflow
    // N: Set
    // C: Set if borrowed
    u32 t = (u32)HL() - reg - (F() & F_CARRY);
    u8 x = (u8)(((HL() & 0x8800) >> 11) | ((reg & 0x8800) >> 10) | ((t & 0x8800) >> 9));
    MP() = HL() + 1;
    HL() = (u16)t;
    F() = ((t & 0x10000) ? F_CARRY : 0) | F_NEG | kOverflowSub[x >> 4] | (H() & (F_3 | F_5 | F_SIGN))
        | kHalfCarrySub[x & 0x07] | (HL() ? 0 : F_ZERO);
}

void Z80::cpReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Borrow from 4 during 'subtraction'
    // P: Overflow (r > A)
    // N: Set
    // C: Set if borrowed (r > A)
    u16 t = (int)A() - reg;
    u8 x = (u8)(((A() & 0x88) >> 3) | ((reg & 0x88) >> 2) | ((t & 0x88) >> 1));
    F() = ((t & 0x100) ? F_CARRY : (t ? 0 : F_ZERO)) | F_NEG | kHalfCarrySub[x & 7] | kOverflowSub[x >> 4] |
        (reg & (F_3 | F_5)) | ((u8)t & F_SIGN);
}

void Z80::andReg8(u8& reg)
{
    A() &= reg;

    // S, Z: Based on result
    // H: Set
    // P: Overflow
    // N: Reset
    // C: Reset
    F() = F_HALF | m_SZ53P[A()];
}

void Z80::orReg8(u8& reg)
{
    A() |= reg;

    // S, Z: Based on result
    // H: Reset
    // P: Overflow
    // N: Reset
    // C: Reset
    F() = m_SZ53P[A()];
}

void Z80::xorReg8(u8& reg)
{
    A() ^= reg;

    // S, Z: Based on result
    // H: Reset
    // P: Overflow
    // N: Reset
    // C: Reset
    F() = m_SZ53P[A()];
}

//         +-------------------------------------+
//  +---+  |  +---+---+---+---+---+---+---+---+  |
//  | C |<-+--| 7                           0 |<-+
//  +---+     +---+---+---+---+---+---+---+---+
//
void Z80::rlcReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    reg = ((reg << 1) | (reg >> 7));
    F() = (reg & F_CARRY) | m_SZ53P[reg];
}

//  +-------------------------------------+
//  |  +---+---+---+---+---+---+---+---+  |  +---+
//  +->| 7                           0 |--+->| C |
//     +---+---+---+---+---+---+---+---+     +---+
//
void Z80::rrcReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F() = reg & F_CARRY;
    reg = ((reg >> 1) | (reg << 7));
    F() |= m_SZ53P[reg];
}

//  +-----------------------------------------------+
//  |  +---+     +---+---+---+---+---+---+---+---+  |
//  +--| C |<----| 7                           0 |<-+
//     +---+     +---+---+---+---+---+---+---+---+
//
void Z80::rlReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    u8 t = reg;
    reg = ((reg << 1) | (F() & F_CARRY));
    F() = (t >> 7) | m_SZ53P[reg];
}

//  +-----------------------------------------------+
//  |  +---+---+---+---+---+---+---+---+     +---+  |
//  +->| 7                           0 |---->| C |--+
//     +---+---+---+---+---+---+---+---+     +---+
//
void Z80::rrReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    u8 t = reg;
    reg = ((reg >> 1) | (F() << 7));
    F() = (t & F_CARRY) | m_SZ53P[reg];
}

//  +---+     +---+---+---+---+---+---+---+---+
//  | C |<----| 7                           0 |<---- 0
//  +---+     +---+---+---+---+---+---+---+---+
//
void Z80::slaReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    F() = reg >> 7;
    reg = (reg << 1);
    F() |= m_SZ53P[reg];
}

//     +---+---+---+---+---+---+---+---+     +---+
//  +--| 7                           0 |---->| C |
//  |  +---+---+---+---+---+---+---+---+     +---+
//  |    ^
//  |    |
//  +----+
//
void Z80::sraReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F() = reg & F_CARRY;
    reg = ((reg & 0x80) | (reg >> 1));
    F() |= m_SZ53P[reg];
}

//  +---+     +---+---+---+---+---+---+---+---+
//  | C |<----| 7                           0 |<---- 1
//  +---+     +---+---+---+---+---+---+---+---+
//
void Z80::sl1Reg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    F() = reg >> 7;
    reg = ((reg << 1) | 0x01);
    F() |= m_SZ53P[reg];
}

//         +---+---+---+---+---+---+---+---+     +---+
//  0 ---->| 7                           0 |---->| C |
//         +---+---+---+---+---+---+---+---+     +---+
//
void Z80::srlReg8(u8& reg)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F() = reg & F_CARRY;
    reg = (reg >> 1);
    F() |= m_SZ53P[reg];
}

void Z80::bitReg8(u8& reg, int b)
{
    // S: Undefined (set to bit 7 if bit 7 is checked, otherwise 0)
    // Z: Opposite of bit b
    // H: Set
    // P: Undefined (same as Z)
    // N: Reset
    // C: Preserved
    F() = (F() & F_CARRY) | F_HALF | (reg & (F_3 | F_5));
    if (!(reg & (1 << b))) F() |= F_PARITY | F_ZERO;
    if ((b == 7) && (reg & 0x80)) F() |= F_SIGN;
}

void Z80::bitReg8MP(u8& reg, int b)
{
    // S: Undefined (set to bit 7 if bit 7 is checked, otherwise 0)
    // Z: Opposite of bit b
    // H: Set
    // P: Undefined (same as Z)
    // N: Reset
    // C: Preserved
    F() = (F() & F_CARRY) | F_HALF | (m_mp.h & (F_3 | F_5));
    if (!(reg & (1 << b))) F() |= F_PARITY | F_ZERO;
    if ((b == 7) && (reg & 0x80)) F() |= F_SIGN;
}

void Z80::resReg8(u8& reg, int b)
{
    // All flags preserved.
    reg = reg & ~((u8)1 << b);
}

void Z80::setReg8(u8& reg, int b)
{
    // All flags preserved.
    reg = reg | ((u8)1 << b);
}

void Z80::daa()
{
    u8 result = A();
    u8 incr = 0;
    bool carry = (F() & F_CARRY) != 0;

    if (((F() & F_HALF) != 0) || ((result & 0x0f) > 0x09))
    {
        incr |= 0x06;
    }

    if (carry || (result > 0x9f) || ((result > 0x8f) && ((result & 0x0f) > 0x09)))
    {
        incr |= 0x60;
    }

    if (result > 0x99) carry = true;

    if ((F() & F_NEG) != 0)
    {
        subReg8(incr);
    }
    else
    {
        addReg8(incr);
    }

    result = A();

    setFlags(F_CARRY, carry);
    setFlags(F_PARITY, m_parity[result] != 0);
}

int Z80::displacement(u8 x)
{
    return (128 ^ (int)x) - 128;
}

u16 Z80::pop(i64& inOutTState)
{
    u16 x = m_ext.peek16(SP(), inOutTState);
    SP() += 2;
    return x;
}

void Z80::push(u16 x, i64& inOutTState)
{
    Reg r(x);
    m_ext.poke(--SP(), r.h, inOutTState);
    m_ext.poke(--SP(), r.l, inOutTState);
}

//----------------------------------------------------------------------------------------------------------------------
// z80Step
// Run a single instruction
//----------------------------------------------------------------------------------------------------------------------

u8& Z80::getReg8(u8 y)
{
    static u8 dummy = 0;

    switch (y)
    {
    case 0:     return B();
    case 1:     return C();
    case 2:     return D();
    case 3:     return E();
    case 4:     return H();
    case 5:     return L();
    case 7:     return A();

    default:    assert(0);
                return dummy;
    }
}

u16& Z80::getReg16_1(u8 p)
{
    switch (p)
    {
    case 0: return BC();
    case 1: return DE();
    case 2: return HL();
    case 3: return SP();
    }

    static u16 _;
    return _;
}


u16& Z80::getReg16_2(u8 p)
{
    switch (p)
    {
    case 0: return BC();
    case 1: return DE();
    case 2: return HL();
    case 3: return AF();
    }

    static u16 _;
    return _;
}

bool Z80::getFlag(u8 y, u8 flags)
{
    switch (y)
    {
    case 0: return (!(flags & F_ZERO)) != 0;
    case 1: return (flags & F_ZERO) != 0;
    case 2: return (!(flags & F_CARRY)) != 0;
    case 3: return (flags & F_CARRY) != 0;
    case 4: return (!(flags & F_PARITY)) != 0;
    case 5: return (flags & F_PARITY) != 0;
    case 6: return (!(flags & F_SIGN)) != 0;
    case 7: return (flags & F_SIGN) != 0;
    }

    return true;
}


Z80::ALUFunc Z80::getAlu(u8 y)
{
    static ALUFunc funcs[8] =
    {
        std::bind(&Z80::addReg8, this, std::placeholders::_1),
        std::bind(&Z80::adcReg8, this, std::placeholders::_1),
        std::bind(&Z80::subReg8, this, std::placeholders::_1),
        std::bind(&Z80::sbcReg8, this, std::placeholders::_1),
        std::bind(&Z80::andReg8, this, std::placeholders::_1),
        std::bind(&Z80::xorReg8, this, std::placeholders::_1),
        std::bind(&Z80::orReg8, this, std::placeholders::_1),
        std::bind(&Z80::cpReg8, this, std::placeholders::_1),
    };

    return funcs[y];
};

Z80::RotShiftFunc Z80::getRotateShift(u8 y)
{
    static RotShiftFunc funcs[8] =
    {
        std::bind(&Z80::rlcReg8, this, std::placeholders::_1),
        std::bind(&Z80::rrcReg8, this, std::placeholders::_1),
        std::bind(&Z80::rlReg8, this, std::placeholders::_1),
        std::bind(&Z80::rrReg8, this, std::placeholders::_1),
        std::bind(&Z80::slaReg8, this, std::placeholders::_1),
        std::bind(&Z80::sraReg8, this, std::placeholders::_1),
        std::bind(&Z80::sl1Reg8, this, std::placeholders::_1),
        std::bind(&Z80::srlReg8, this, std::placeholders::_1),
    };

    return funcs[y];
}


#define PEEK(a) m_ext.peek((a), tState)
#define POKE(a, b) m_ext.poke((a), (b), tState)
#define PEEK16(a) m_ext.peek16((a), tState)
#define POKE16(a, w) m_ext.poke16((a), (w), tState)
#define CONTEND(a, t, n) m_ext.contend((a), (t), (n), tState)

void Z80::decodeInstruction(u8 opCode, u8& x, u8& y, u8& z, u8& p, u8& q)
{
    x = (opCode & 0xc0) >> 6;
    y = (opCode & 0x38) >> 3;
    z = (opCode & 0x07);
    p = (y & 6) >> 1;
    q = (y & 1);
}

u8 Z80::fetchInstruction(i64& tState)
{
    // Fetch opcode and decode it.  The opcode can be viewed as XYZ fields with Y being sub-decoded to PQ fields:
    //
    //    7   6   5   4   3   2   1   0
    //  +---+---+---+---+---+---+---+---+
    //  |   X   |     Y     |     Z     |
    //  +---+---+---+---+---+---+---+---+
    //  |       |   P   | Q |           |
    //  +---+---+---+---+---+---+---+---+
    //
    //
    // See http://www.z80.info/decoding.htm
    //
    u8 r = R();
    R() = (r & 0x80) | ((r + 1) & 0x7f);
    CONTEND(PC(), 4, 1);
    return m_ext.peek(PC()++);
}

//----------------------------------------------------------------------------------------------------------------------
// Basic opcode interpretation
//----------------------------------------------------------------------------------------------------------------------

#define II idx.r
#define IH idx.h
#define IL idx.l

void Z80::executeDDFDCB(Reg& idx, i64& tState)
{
    u8 x, y, z;
    u8 opCode;

    CONTEND(PC(), 3, 1);
    MP() = II + (i8)m_ext.peek(PC());
    ++PC();
    CONTEND(PC(), 3, 1);
    opCode = m_ext.peek(PC());
    CONTEND(PC(), 1, 2);
    ++PC();

    x = (opCode & 0xc0) >> 6;
    y = (opCode & 0x38) >> 3;
    z = (opCode & 0x07);

    u8* r = 0;
    u8 v = 0;

    switch (x)
    {
    case 0:     // LD R[z],rot/shift[y] (IX+d)      or rot/shift[y] (IX+d) (z == 6)
        r = (z == 6) ? &v : &getReg8(z);
        *r = PEEK(MP());
        CONTEND(MP(), 1, 1);
        getRotateShift(y)(*r);
        POKE(MP(), *r);
        break;

    case 1:     // BIT y,(IX+d)
        v = PEEK(MP());
        CONTEND(MP(), 1, 1);
        bitReg8MP(v, y);
        break;

    case 2:     // LD R[z],RES y,(IX+d)             or RES y,(IX+d)  (z == 6)
        r = (z == 6) ? &v : &getReg8(z);
        *r = PEEK(MP());
        resReg8(*r, y);
        CONTEND(MP(), 1, 1);
        POKE(MP(), *r);
        break;

    case 3:     // LD R[z],SET y,(IX+d)             or SET y,(IX+d)  (z == 6)
        r = (z == 6) ? &v : &getReg8(z);
        *r = PEEK(MP());
        setReg8(*r, y);
        CONTEND(MP(), 1, 1);
        POKE(MP(), *r);
        break;
    }
}

void Z80::executeDDFD(Reg& idx, i64& tState)
{
    u8 x, y, z, p, q;
    u8 opCode = fetchInstruction(tState);
    decodeInstruction(opCode, x, y, z, p, q);

    i8 d = 0;
    u8 v = 0;
    u16 tt = 0;
    u8* r = 0;
    u16* rr = 0;

    switch (x)
    {
    case 0: // x = 0
        switch (z)
        {
        case 1:
            if (0 == q)
            {
                // 21 - LD IX,nn
                if (2 == p)
                {
                    II = PEEK16(PC());
                    PC() += 2;
                }
                else goto invalid_instruction;
            }
            else
            {
                // 09 19 29 39 - ADD IX,BC/DE/IX/SP
                CONTEND(IR(), 1, 7);
                MP() = II + 1;
                rr = &getReg16_1(p);
                rr = (rr == &HL()) ? &II : rr;
                addReg16(II, *rr);
            }
            break;

        case 2:
            if (2 == p)
            {
                if (0 == q)
                {
                    // 22 - LD (nn),IX
                    tt = PEEK16(PC());
                    POKE16(tt, II);
                    MP() = tt + 1;
                    PC() += 2;
                }
                else
                {
                    // 2A - LD IX,(nn)
                    tt = PEEK16(PC());
                    II = PEEK16(tt);
                    PC() += 2;
                    MP() = tt + 1;
                }
            }
            else
            {
                goto invalid_instruction;
            }
            break;

        case 3:
            if (p == 2)
            {
                if (0 == q)
                {
                    // 23 - INC IX
                    CONTEND(IR(), 1, 2);
                    ++II;
                }
                else
                {
                    // 2B - DEC IX
                    CONTEND(IR(), 1, 2);
                    --II;
                }
            }
            else goto invalid_instruction;
            break;

        case 4:
            switch (y)
            {
            case 4: // 24 - INC IXH
                incReg8(IH);
                break;

            case 5: // 2C - INC IXL
                incReg8(IL);
                break;

            case 6: // 34 - INC (IX+d)
                d = PEEK(PC());
                CONTEND(PC(), 1, 5);
                ++PC();
                MP() = II + d;
                v = PEEK(MP());
                CONTEND(MP(), 1, 1);
                incReg8(v);
                POKE(MP(), v);
                break;

            default:
                goto invalid_instruction;
            }
            break;

        case 5:
            switch (y)
            {
            case 4: // 25 - DEC IXH
                decReg8(IH);
                break;

            case 5: // 2D - DEC IXL
                decReg8(IL);
                break;

            case 6: // 35 - DEC (IX+d)
                d = PEEK(PC());
                CONTEND(PC(), 1, 5);
                ++PC();
                MP() = II + d;
                v = PEEK(MP());
                CONTEND(MP(), 1, 1);
                decReg8(v);
                POKE(MP(), v);
                break;

            default:
                goto invalid_instruction;
            }
            break;

        case 6: // z = 6
            switch (y)
            {
            case 4: // 26 - LD IXH,n
                IH = PEEK(PC()++);
                break;

            case 5: // 2E - LD IXL,n
                IL = PEEK(PC()++);
                break;

            case 6: // 36 - LD (IX+d),n
                d = PEEK(PC()++);
                v = PEEK(PC());
                CONTEND(PC(), 1, 2);
                ++PC();
                MP() = II + d;
                POKE(MP(), v);
                break;

            default:
                goto invalid_instruction;
            }
            break;

        default:
            goto invalid_instruction;
        } // switch(z), x = 0
        break;

    case 1: // x = 1
        // LD R,R
        if (y != 6 || z != 6)
        {
            u8* r1 = 0;
            u8* r2 = 0;

            switch (y)
            {
            case 4:     // LD IXH,R
            case 5:     // LD IXL,R
                r1 = y == 4 ? &IH : &IL;
                switch (z)
                {
                case 4:     r2 = &IH;   break;  // 64/6C - LD IXH/IXL,IXH
                case 5:     r2 = &IL;   break;  // 65/6D - LD IXH/IXL,IXL
                case 6:
                    // 66/6E - LD H/L,(IX+d)
                    r1 = &getReg8(y);
                    d = PEEK(PC());
                    CONTEND(PC(), 1, 5);
                    ++PC();
                    MP() = II + d;
                    x = PEEK(MP());
                    r2 = &x;
                    break;

                default:
                    // LD IXH,R
                    r2 = &getReg8(z);
                }
                break;

            case 6:     // LD (IX+d),R
                d = PEEK(PC());
                CONTEND(PC(), 1, 5);
                ++PC();
                MP() = II + d;
                POKE(MP(), getReg8(z));
                break;

            default:
                switch (z)
                {
                case 4:     // LD R,IXH
                case 5:     // LD R,IXL
                    r2 = z == 4 ? &IH : &IL;
                    r1 = &getReg8(y);
                    break;

                case 6:     // LD R,(IX+d)
                    r1 = &getReg8(y);
                    d = PEEK(PC());
                    CONTEND(PC(), 1, 5);
                    ++PC();
                    MP() = II + d;
                    x = PEEK(MP());
                    r2 = &x;
                    break;

                default:
                    goto invalid_instruction;
                }
            }
            if (r1) *r1 = *r2;
        }
        else goto invalid_instruction;
        break;

    case 2: // x = 2
        switch (z)
        {
        case 4: r = &IH; break;
        case 5: r = &IL; break;
        case 6:
            d = PEEK(PC());
            CONTEND(PC(), 1, 5);
            ++PC();
            MP() = II + d;
            v = PEEK(MP());
            r = &v;
            break;

        default:
            goto invalid_instruction;
        }

        getAlu(y)(*r);
        break;

    case 3: // x = 3
        switch (opCode)
        {
        case 0xcb:  // DDCB prefixes
            executeDDFDCB(idx, tState);
            break;

        case 0xe1:  // POP IX
            II = pop(tState);
            break;

        case 0xe3:  // EX (SP),IX
            {
                Reg t;
                t.l = PEEK(SP());
                t.h = PEEK(SP() + 1);
                CONTEND(SP() + 1, 1, 1);
                POKE(SP() + 1, IH);
                POKE(SP(), IL);
                CONTEND(SP(), 1, 2);
                II = MP() = t.r;
            }
            break;

        case 0xe5:  // PUSH IX
            CONTEND(IR(), 1, 1);
            push(II, tState);
            break;

        case 0xe9:  // JP (IX)
            PC() = II;
            break;

        case 0xf9:  // LD (SP),IX
            CONTEND(IR(), 1, 2);
            SP() = II;
            break;

        default:
            goto invalid_instruction;
        }
        break;
    }

    return;

invalid_instruction:
    execute(opCode, tState);
    return;
}

void Z80::executeED(i64& tState)
{
    u8 x, y, z, p, q;
    u8 opCode = fetchInstruction(tState);
    decodeInstruction(opCode, x, y, z, p, q);

    u8* r = 0;
    u8 v = 0;

    switch (x)
    {
    case 0:     // x = 0: 00-3F
        goto invalid_instruction;

    case 1:     // x = 1: 40-7F
        switch (z)
        {
        case 0:
            r = (y == 6) ? &v : &getReg8(y);
            MP() = BC() + 1;
            *r = m_ext.in(BC(), tState);
            NX_LOG_IN(BC(), *r);
            F() = (F() & F_CARRY) | m_SZ53P[*r];
            break;

        case 1:
            v = 0;
            r = (y == 6) ? &v : &getReg8(y);
            NX_LOG_OUT(BC(), *r);
            m_ext.out(BC(), *r, tState);
            MP() = BC() + 1;
            break;

        case 2:
            if (0 == q)
            {
                // SBC HL,RR
                CONTEND(IR(), 1, 7);
                sbcReg16(getReg16_1(p));
            }
            else
            {
                // ADC HL,RR
                CONTEND(IR(), 1, 7);
                adcReg16(getReg16_1(p));
            }
            break;

        case 3:
            if (0 == q)
            {
                // LD (nn),RR
                u16 tt = PEEK16(PC());
                u16* rr = &getReg16_1(p);
                PC() += 2;
                POKE16(tt, *rr);
                MP() = tt + 1;
            }
            else
            {
                // LD RR,(nn)
                u16 tt = PEEK16(PC());
                u16* rr = &getReg16_1(p);
                PC() += 2;
                *rr = PEEK16(tt);
                MP() = tt + 1;
            }
            break;

        case 4: // NEG
            v = A();
            A() = 0;
            subReg8(v);
            break;

        case 5: // RETI & RETN
            IFF1() = IFF2();
            PC() = pop(tState);
            MP() = PC();
            break;

        case 6: // IM ?
            y &= 3;
            IM() = (y == 0) ? 0 : y - 1;
            break;

        case 7:
            switch (y)
            {
            case 0: // LD I,A()
                CONTEND(IR(), 1, 1);
                I() = A();
                break;

            case 1: // LD R,A()
                CONTEND(IR(), 1, 1);
                R() = A();
                break;

            case 2: // LD A(),I
                CONTEND(IR(), 1, 1);
                A() = I();
                F() = (F() & F_CARRY) | m_SZ53[A()] | (IFF2() ? F_PARITY : 0);
                // #todo: handle IFF2 event
                break;

            case 3: // LD A(),R
                CONTEND(IR(), 1, 1);
                A() = R();
                F() = (F() & F_CARRY) | m_SZ53[A()] | (IFF2() ? F_PARITY : 0);
                break;

            case 4: // RRD
                v = PEEK(HL());
                CONTEND(HL(), 1, 4);
                POKE(HL(), (A() << 4) | (v >> 4));
                A() = (A() & 0xf0) | (v & 0x0f);
                F() = (F() & F_CARRY) | m_SZ53P[A()];
                MP() = HL() + 1;
                break;

            case 5: // RLD
                v = PEEK(HL());
                CONTEND(HL(), 1, 4);
                POKE(HL(), (v << 4) | (A() & 0x0f));
                A() = (A() & 0xf0) | (v >> 4);
                F() = (F() & F_CARRY) | m_SZ53P[A()];
                MP() = HL() + 1;
                break;

            case 6:
            case 7: // NOP
                break;
            }
            break;
        }
        break;

    case 2:     // x = 2: 80-BF
        switch (opCode)
        {
        case 0xa0:  // LDI
            v = PEEK(HL());
            --BC();
            POKE(DE(), v);
            CONTEND(DE(), 1, 2);
            ++DE();
            ++HL();
            v += A();
            F() = (F() & (F_CARRY | F_ZERO | F_SIGN)) | (BC() ? F_PARITY : 0) |
                (v & F_3) | ((v & 0x02) ? F_5 : 0);
            break;

        case 0xa1:  // CPI
            {
                v = PEEK(HL());
                u8 t = A() - v;
                u8 lookup = ((A() & 0x08) >> 3) | ((v & 0x08) >> 2) | ((t & 0x08) >> 1);
                CONTEND(HL(), 1, 5);
                ++HL();
                --BC();
                F() = (F() & F_CARRY) | (BC() ? (F_PARITY | F_NEG) : F_NEG) |
                    kHalfCarrySub[lookup] | (t ? 0 : F_ZERO) | (t & F_SIGN);
                if (F() & F_HALF) --t;
                F() |= (t & F_3) | ((t & 0x02) ? F_5 : 0);
                ++MP();
            }
            break;

        case 0xa2:  // INI
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = m_ext.in(BC(), tState);
                NX_LOG_IN(BC(), t1);
                POKE(HL(), t1);
                MP() = BC() + 1;
                --B();
                ++HL();
                t2 = t1 + C() + 1;
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
            }
            break;

        case 0xa3:  // OUTI
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = PEEK(HL());
                --B();
                MP() = BC() + 1;
                m_ext.out(BC(), t1, tState);
                ++HL();
                t2 = t1 + L();
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
            }
            break;

        case 0xa8:  // LDD
            v = PEEK(HL());
            --BC();
            POKE(DE(), v);
            CONTEND(DE(), 1, 2);
            --DE();
            --HL();
            v += A();
            F() = (F() & (F_CARRY | F_ZERO | F_SIGN)) | (BC() ? F_PARITY : 0) |
                (v & F_3) | ((v & 0x02) ? F_5 : 0);
            break;

        case 0xa9:  // CPD
            {
                v = PEEK(HL());
                u8 t = A() - v;
                u8 lookup = ((A() & 0x08) >> 3) | ((v & 0x08) >> 2) | ((t & 0x08) >> 1);
                CONTEND(HL(), 1, 5);
                --HL();
                --BC();
                F() = (F() & F_CARRY) | (BC() ? (F_PARITY | F_NEG) : F_NEG) |
                    kHalfCarrySub[lookup] | (t ? 0 : F_ZERO) | (t & F_SIGN);
                if (F() & F_HALF) --t;
                F() |= (t & F_3) | ((t & 0x02) ? F_5 : 0);
                --MP();
            }
            break;

        case 0xaa:  // IND
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = m_ext.in(BC(), tState);
                NX_LOG_IN(BC(), t1);
                POKE(HL(), t1);
                MP() = BC() - 1;
                --B();
                --HL();
                t2 = t1 + C() - 1;
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
            }
            break;

        case 0xab:  // OUTD
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = PEEK(HL());
                --B();
                MP() = BC() - 1;
                NX_LOG_OUT(BC(), t1);
                m_ext.out(BC(), t1, tState);
                --HL();
                t2 = t1 + L();
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
            }
            break;

        case 0xb0:  // LDIR
            v = PEEK(HL());
            POKE(DE(), v);
            CONTEND(DE(), 1, 2);
            --BC();
            v += A();
            F() = (F() & (F_CARRY | F_ZERO | F_SIGN)) | (BC() ? F_PARITY : 0) |
                (v & F_3) | ((v & 0x02) ? F_5 : 0);
            if (BC())
            {
                CONTEND(DE(), 1, 5);
                PC() -= 2;
                MP() = PC() + 1;
            }
            ++DE();
            ++HL();
            break;

        case 0xb1:  // CPIR
            {
                v = PEEK(HL());
                u8 t = A() - v;
                u8 lookup = ((A() & 0x08) >> 3) | ((v & 0x08) >> 2) | ((t & 0x08) >> 1);
                CONTEND(HL(), 1, 5);
                --BC();
                F() = (F() & F_CARRY) | (BC() ? (F_PARITY | F_NEG) : F_NEG) |
                    kHalfCarrySub[lookup] | (t ? 0 : F_ZERO) | (t & F_SIGN);
                if (F() & F_HALF) --t;
                F() |= (t & F_3) | ((t & 0x02) ? F_5 : 0);
                if ((F() & (F_PARITY | F_ZERO)) == F_PARITY)
                {
                    CONTEND(HL(), 1, 5);
                    PC() -= 2;
                    MP() = PC() + 1;
                }
                else
                {
                    ++MP();
                }
                ++HL();
            }
            break;

        case 0xb2:  // INIR
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = m_ext.in(BC(), tState);
                NX_LOG_IN(BC(), t1);
                POKE(HL(), t1);
                MP() = BC() + 1;
                --B();
                t2 = t1 + C() + 1;
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
                if (B())
                {
                    CONTEND(HL(), 1, 5);
                    PC() -= 2;
                }
                ++HL();
            }
            break;

        case 0xb3:  // OTIR
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = PEEK(HL());
                --B();
                MP() = BC() + 1;
                NX_LOG_OUT(BC(), t1);
                m_ext.out(BC(), t1, tState);
                ++HL();
                t2 = t1 + L();
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
                if (B())
                {
                    CONTEND(BC(), 1, 5);
                    PC() -= 2;
                }
            }
            break;

        case 0xb8:  // LDDR
            v = PEEK(HL());
            POKE(DE(), v);
            CONTEND(DE(), 1, 2);
            --BC();
            v += A();
            F() = (F() & (F_CARRY | F_ZERO | F_SIGN)) | (BC() ? F_PARITY : 0) |
                (v & F_3) | ((v & 0x02) ? F_5 : 0);
            if (BC())
            {
                CONTEND(DE(), 1, 5);
                PC() -= 2;
                MP() = PC() + 1;
            }
            --DE();
            --HL();
            break;

        case 0xb9:  // CPDR
            {
                v = PEEK(HL());
                u8 t = A() - v;
                u8 lookup = ((A() & 0x08) >> 3) | ((v & 0x08) >> 2) | ((t & 0x08) >> 1);
                CONTEND(HL(), 1, 5);
                --BC();
                F() = (F() & F_CARRY) | (BC() ? (F_PARITY | F_NEG) : F_NEG) |
                    kHalfCarrySub[lookup] | (t ? 0 : F_ZERO) | (t & F_SIGN);
                if (F() & F_HALF) --t;
                F() |= (t & F_3) | ((t & 0x02) ? F_5 : 0);
                if ((F() & (F_PARITY | F_ZERO)) == F_PARITY)
                {
                    CONTEND(HL(), 1, 5);
                    PC() -= 2;
                    MP() = PC() + 1;
                }
                else
                {
                    --MP();
                }
                --HL();
            }
            break;

        case 0xba:  // INDR
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = m_ext.in(BC(), tState);
                NX_LOG_IN(BC(), t1);
                POKE(HL(), t1);
                MP() = BC() - 1;
                --B();
                t2 = t1 + C() - 1;
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
                if (B())
                {
                    CONTEND(HL(), 1, 5);
                    PC() -= 2;
                }
                --HL();
            }
            break;

        case 0xbb:  // OTDR
            {
                u8 t1, t2;
                CONTEND(IR(), 1, 1);
                t1 = PEEK(HL());
                --B();
                MP() = BC() - 1;
                NX_LOG_OUT(BC(), t1);
                m_ext.out(BC(), t1, tState);
                --HL();
                t2 = t1 + L();
                F() = (t1 & 0x80 ? F_NEG : 0) |
                    ((t2 < t1) ? F_HALF | F_CARRY : 0) |
                    (m_parity[(t2 & 0x07) ^ B()] ? F_PARITY : 0) |
                    m_SZ53[B()];
                if (B())
                {
                    CONTEND(BC(), 1, 5);
                    PC() -= 2;
                }
            }
            break;

        default:
            goto invalid_instruction;
        }
        break;

    case 3:     // x = 3: C0-FF
        goto invalid_instruction;
    }

    return;

invalid_instruction:
    execute(opCode, tState);
    return;
}

void Z80::execute(u8 opCode, i64& tState)
{
    u8 x, y, z, p, q;
    decodeInstruction(opCode, x, y, z, p, q);

    // Commonly used local variables
    u8 d = 0;       // Used for displacement
    u16 tt = 0;     // Temporary for an address
    u8* r;          // Temporary for a reference

    // Opcode hex calculated from:
    //
    //      X = $00, $40, $80, $c0
    //      Y = add: $08, $10, $18, $20, $28, $30, $38
    //      Z = add: Z
    //      P = add: $00, $10, $20, $30
    //      Q = add: $00, $08

    switch (x)
    {
    case 0:
        switch (z)
        {
        case 0: // x, z = (0, 0)
            switch (y)
            {
            case 0:     // 00 - NOP
                break;

            case 1:     // 08 - EX AF,AF'
                exAfAf();
                break;

            case 2:     // 10 - DJNZ d
                CONTEND(IR(), 1, 1);
                --B();
                if (B() != 0)
                {
                    d = displacement(PEEK(PC()));
                    CONTEND(PC(), 1, 5);
                    PC() += (i8)(d + 1);
                    MP() = PC();
                }
                else
                {
                    CONTEND(PC(), 3, 1);
                    ++PC();
                }
                break;

            case 3:     // 18 - JR d
                d = displacement(PEEK(PC()));
                CONTEND(PC(), 1, 5);
                PC() += (i8)(d + 1);
                MP() = PC();
                break;

            default:    // 20, 28, 30, 38 - JR cc(y-4),d
                if (getFlag(y - 4, F()))
                {
                    d = displacement(PEEK(PC()));
                    CONTEND(PC(), 1, 5);
                    PC() += (i8)(d + 1);
                    MP() = PC();
                }
                else
                {
                    CONTEND(PC(), 3, 1);
                    ++PC();
                }
            }
            break;

        case 1: // x, z = (0, 1)
            if (0 == q)
            {
                // 01, 11, 21, 31 - LD BC()/DE()/HL()/SP(), nnnn
                u16* rr = &getReg16_1(p);
                *rr = PEEK16(PC());
                PC() += 2;
            }
            else
            {
                // 09, 19, 29, 39 - ADD HL(), BC()/DE()/HL()/SP()
                CONTEND(IR(), 1, 7);
                MP() = HL() + 1;
                addReg16(HL(), getReg16_1(p));
            }
            break;

        case 2: // x, z = (0, 2)
            switch (y)
            {
            case 0:     // 02 - LD (BC()),A()
                POKE(BC(), A());
                MP() = (((BC() + 1) & 0xff) | (A() << 8));
                break;

            case 1:     // 0A - LD A(),(BC())
                A() = PEEK(BC());
                MP() = BC() + 1;
                break;

            case 2:     // 12 - LD (DE()),A()
                POKE(DE(), A());
                MP() = (((DE() + 1) & 0xff) | (A() << 8));
                break;

            case 3:     // 1A - LD A(),(DE())
                A() = PEEK(DE());
                MP() = DE() + 1;
                break;

            case 4:     // 22 - LD (nn),HL()
                tt = PEEK16(PC());
                POKE16(tt, HL());
                MP() = tt + 1;
                PC() += 2;
                break;

            case 5:     // 2A - LD HL(),(nn)
                tt = PEEK16(PC());
                HL() = PEEK16(tt);
                PC() += 2;
                MP() = tt + 1;
                break;

            case 6:     // 32 - LD (nn),A()
                tt = PEEK16(PC());
                PC() += 2;
                POKE(tt, A());
                m_mp.l = (u8)(tt + 1);
                m_mp.h = A();
                break;

            case 7:     // 3A - LD A(),(nn)
                tt = PEEK16(PC());
                MP() = tt + 1;
                A() = PEEK(tt);
                PC() += 2;
                break;
            }
            break;

        case 3: // x, z = (0, 3)
            if (0 == q)
            {
                // 03, 13, 23, 33 - INC BC()/DE()/HL()/SP()
                CONTEND(IR(), 1, 2);
                ++(*&getReg16_1(p));
            }
            else
            {
                // 0B, 1B, 2B, 3B - DEC BC()/DE()/HL()/SP()
                CONTEND(IR(), 1, 2);
                --(*&getReg16_1(p));
            }
            break;

        case 4: // x, z = (0, 4)
            // 04, 0C, 14, 1C, 24, 2C, 34, 3C - INC B()/C()/D/E/H/L()/(HL())/A()
            if (y == 6)
            {
                d = PEEK(HL());
                CONTEND(HL(), 1, 1);
                incReg8(d);
                POKE(HL(), d);
            }
            else
            {
                incReg8(getReg8(y));
            }
            break;

        case 5: // x, z = (0, 5)
            // 05, 0D, 15, 1D, 25, 2D, 35, 3D - DEC B()/C()/D/E/H/L()/(HL())/A()
            if (y == 6)
            {
                d = PEEK(HL());
                CONTEND(HL(), 1, 1);
                decReg8(d);
                POKE(HL(), d);
            }
            else
            {
                decReg8(getReg8(y));
            }
            break;

        case 6: // x, z = (0, 6)
            // 06, 0E, 16, 1E, 26, 2E, 36, 3E - LD B()/C()/D/E/H/L()/(HL())/A(), n
            if (y == 6)
            {
                POKE(HL(), PEEK(PC()++));
            }
            else
            {
                r = &getReg8(y);
                *r = PEEK(PC()++);
            }
            break;

        case 7: // x, z = (0, 7)
            switch (y)
            {
            case 0:     // 07 - RLCA
                A() = ((A() << 1) | (A() >> 7));
                F() = (F() & (F_PARITY | F_ZERO | F_SIGN)) | (A() & (F_CARRY | F_3 | F_5));
                break;

            case 1:     // 0F - RRCA
                F() = (F() & (F_PARITY | F_ZERO | F_SIGN)) | (A() & F_CARRY);
                A() = ((A() >> 1) | (A() << 7));
                F() |= (A() & (F_3 | F_5));
                break;

            case 2:     // 17 - RLA
                d = A();
                A() = (A() << 1) | (F() & F_CARRY);
                F() = (F() & (F_PARITY | F_ZERO | F_SIGN)) | (A() & (F_3 | F_5)) | (d >> 7);
                break;

            case 3:     // 1F - RRA
                d = A();
                A() = (A() >> 1) | (F() << 7);
                F() = (F() & (F_PARITY | F_ZERO | F_SIGN)) | (A() & (F_3 | F_5)) | (d & F_CARRY);
                break;

            case 4:     // 27 - DAA
                daa();
                break;

            case 5:     // 2F - CPL
                A() = A() ^ 0xff;
                F() = (F() & (F_CARRY | F_PARITY | F_ZERO | F_SIGN)) | (A() & (F_3 | F_5)) | F_NEG | F_HALF;
                break;

            case 6:     // 37 - SCF
                F() = (F() & (F_PARITY | F_ZERO | F_SIGN)) | (A() & (F_3 | F_5)) | F_CARRY;
                break;

            case 7:     // 3F - CCF
                F() = (F() & (F_PARITY | F_ZERO | F_SIGN)) | (A() & (F_3 | F_5)) | ((F() & F_CARRY) ? F_HALF : F_CARRY);
                break;
            }
            break;

        } // switch(z)
        break; // x == 0

    case 1:
        if (z == 6 && y == 6)
        {
            // 76 - HALT
            m_halt = true;
            --PC();
        }
        else
        {
            // 40 - 7F - LD R,R
            if (y == 6)
            {
                // LD (HL()),R
                u8* r = &getReg8(z);
                POKE(HL(), *r);
            }
            else if (z == 6)
            {
                // LD R,(HL())
                u8* r = &getReg8(y);
                *r = PEEK(HL());
            }
            else
            {
                u8* r1 = &getReg8(y);
                u8* r2 = &getReg8(z);
                *r1 = *r2;
            }
        }
        break; // x == 1

    case 2:
        {
            if (z == 6)
            {
                // ALU(y) (HL())
                d = PEEK(HL());
                getAlu(y)(d);
            }
            else
            {
                getAlu(y)(getReg8(z));
            }
        }
        break; // x == 2

    case 3:
        switch (z)
        {
        case 0:
            // C0, C8, D0, D8, E0, E8, F0, F8 - RET flag
            CONTEND(IR(), 1, 1);
            if (getFlag(y, F()))
            {
                PC() = pop(tState);
                MP() = PC();
            }
            break;  // x, z = (3, 0)

        case 1:
            if (0 == q)
            {
                // C1, D1, E1, F1 - POP RR
                *&getReg16_2(p) = pop(tState);
            }
            else
            {
                switch (p)
                {
                case 0:     // C9 - RET
                    PC() = pop(tState);
                    MP() = PC();
                    break;

                case 1:     // D9 - EXX
                    exx();
                    break;

                case 2:     // E9 - JP HL()
                    PC() = HL();
                    break;

                case 3:     // F9 - LD SP(), HL()
                    CONTEND(IR(), 1, 2);
                    SP() = HL();
                    break;
                }
            }
            break;  // x, z = (3, 1)

        case 2:
            // C2, CA, D2, DA, E2, EA, F2, FA - JP flag,nn
            tt = PEEK16(PC());
            if (getFlag(y, F()))
            {
                PC() = tt;
            }
            else
            {
                PC() += 2;
            }
            MP() = tt;
            break;  // x, z = (3, 2)

        case 3:
            switch (y)
            {
            case 0:     // C3 - JP nn
                PC() = PEEK16(PC());
                MP() = PC();
                break;

            case 1:     // CB (prefix)
                opCode = fetchInstruction(tState);
                decodeInstruction(opCode, x, y, z, p, q);

                switch (x)
                {
                case 0:     // 00-3F: Rotate/Shift instructions
                    if (z == 6)
                    {
                        d = PEEK(HL());
                        CONTEND(HL(), 1, 1);
                        getRotateShift(y)(d);
                        POKE(HL(), d);
                    }
                    else
                    {
                        getRotateShift(y)(getReg8(z));
                    }
                    break;

                case 1:     // 40-7F: BIT instructions
                    if (z == 6)
                    {
                        // BIT n,(HL())
                        d = PEEK(HL());
                        CONTEND(HL(), 1, 1);
                        bitReg8MP(d, y);
                    }
                    else
                    {
                        bitReg8(getReg8(z), y);
                    }
                    break;

                case 2:     // 80-BF: RES instructions
                    if (z == 6)
                    {
                        // RES n,(HL())
                        d = PEEK(HL());
                        CONTEND(HL(), 1, 1);
                        resReg8(d, y);
                        POKE(HL(), d);
                    }
                    else
                    {
                        resReg8(getReg8(z), y);
                    }
                    break;

                case 3:     // C0-FF: SET instructions
                    if (z == 6)
                    {
                        // BIT n,(HL())
                        d = PEEK(HL());
                        CONTEND(HL(), 1, 1);
                        setReg8(d, y);
                        POKE(HL(), d);
                    }
                    else
                    {
                        setReg8(getReg8(z), y);
                    }
                }
                break;

            case 2:     // D3 - OUT (n),A()       A() -> $AAnn
                d = PEEK(PC());
                NX_LOG_OUT((u16)d | ((u16)A() << 8), A());
                m_ext.out((u16)d | ((u16)A() << 8), A(), tState);
                m_mp.h = A();
                m_mp.l = (u8)(d + 1);
                ++PC();
                break;

            case 3:     // DB - IN A(),(n)        A() <- $AAnn
                d = PEEK(PC());
                tt = ((u16)A() << 8) | d;
                m_mp.h = A();
                m_mp.l = (u8)(d + 1);
                A() = m_ext.in(tt, tState);
                NX_LOG_IN(tt, A());
                ++PC();
                break;

            case 4:     // E3 - EX (SP()),HL()
                tt = PEEK16(SP());
                CONTEND(SP() + 1, 1, 1);
                POKE(SP() + 1, H());
                POKE(SP(), L());
                CONTEND(SP(), 1, 2);
                HL() = tt;
                MP() = HL();
                break;

            case 5:     // EB - EX DE(),HL()
                tt = DE();
                DE() = HL();
                HL() = tt;
                break;

            case 6:     // F3 - DI
                IFF1() = false;
                IFF2() = false;
                break;

            case 7:     // FB - EI
                IFF1() = true;
                IFF2() = true;
                m_eiHappened = true;
                break;
            }
            break;  // x, z = (3, 3)

        case 4:
            // C4 CC D4 DC E4 EC F4 FC - CALL F(),nn
            tt = PEEK16(PC());
            MP() = tt;
            if (getFlag(y, F()))
            {
                CONTEND(PC() + 1, 1, 1);
                push(PC() + 2, tState);
                PC() = tt;
            }
            else
            {
                PC() += 2;
            }
            break;  // x, z = (3, 4)

        case 5:
            if (0 == q)
            {
                // C5 D5 E5 F5 - PUSH RR
                CONTEND(IR(), 1, 1);
                push(*&getReg16_2(p), tState);
            }
            else
            {
                switch (p)
                {
                case 0:     // CD - CALL nn
                    tt = PEEK16(PC());
                    MP() = tt;
                    CONTEND(PC() + 1, 1, 1);
                    push(PC() + 2, tState);
                    PC() = tt;
                    break;

                case 1:     // DD - IX prefix
                    executeDDFD(m_ix, tState);
                    break;

                case 2:     // ED - extensions prefix
                    executeED(tState);
                    break;

                case 3:     // FD - IY prefix
                    executeDDFD(m_iy, tState);
                    break;
                }
            }
            break;  // x, z = (3, 5)

        case 6:
            // C6, CE, D6, DE(), E6, EE, F6, FE - ALU A(),n
            d = PEEK(PC()++);
            getAlu(y)(d);
            break;  // x, z = (3, 6)

        case 7:
            // C7, CF, D7, DF, E7, EF, F7, FF - RST n
            CONTEND(IR(), 1, 1);
            push(PC(), tState);
            PC() = (u16)y * 8;
            MP() = PC();
            break;  // x, z = (3, 7)

        }
        break; // x == 3
    } // switch(x)
}

void Z80::step(i64& tState)
{
    assert(tState >= 0);
    if (IFF1() && /*(*tState < 32)*/ m_interrupt && !m_eiHappened)
    {
        IFF1() = false;
        IFF2() = false;

        if (m_halt)
        {
            m_halt = false;
            ++PC();
        }

        if (m_im < 2)
        {
            push(PC(), tState);
            tState += 7;
            PC() = 0x0038;
        }
        else
        {
            u16 p = (I() << 8) | 0xff;
            push(PC(), tState);
            PC() = m_ext.peek16(p, tState);
            tState += 7;
        }
        MP() = PC();
        m_interrupt = false;
    }
    else
    {
        m_eiHappened = false;
        m_nmi = false;

        u8 opCode = fetchInstruction(tState);
        execute(opCode, tState);
    }
}

void Z80::interrupt()
{
    m_interrupt = true;
}

void Z80::nmi()
{
    m_nmi = true;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
