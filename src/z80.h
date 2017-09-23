//----------------------------------------------------------------------------------------------------------------------
// Emualates the Z80
//----------------------------------------------------------------------------------------------------------------------

#pragma once

typedef struct
{
    union
    {
        u16 r;
        struct {
            u8 l;
            u8 h;
        };
    };
}
Reg;

typedef struct  
{
    // External systems and information.
    Memory*     mem;
    void*       io;         // Placeholder

    // Base registers
    Reg         af, bc, de, hl;
    Reg         sp, pc, ix, iy;
    Reg         ir;

    // Alternate registers
    Reg         af_, bc_, de_, hl_;

    // Internal registers
    Reg         m;

    bool        halt;
    bool        iff1;
    bool        iff2;
    int         im;
    bool        lastOpcodeWasEI;    // YES, if EI was executed (as no interrupt can happen immediately after it)
}
Z80;

// Initialise the internal tables and data structure.
void z80Init(Z80* Z, Memory* mem);

// Run a single opcode
void z80Step(Z80* Z, i64* tState);

// Convenient register accesses that relies on the Z80 structure variable being called Z.
#define A       Z->af.h
#define F       Z->af.l
#define B       Z->bc.h
#define C       Z->bc.l
#define D       Z->de.h
#define E       Z->de.l
#define H       Z->hl.h
#define L       Z->hl.l
#define IXH     Z->ix.h
#define IXL     Z->ix.l
#define IYH     Z->iy.h
#define IYL     Z->iy.l
#define I       Z->ir.h
#define R       Z->ir.l

#define AF      Z->af.r
#define BC      Z->bc.r
#define DE      Z->de.r
#define HL      Z->hl.r
#define SP      Z->sp.r
#define PC      Z->pc.r
#define IX      Z->ix.r
#define IY      Z->iy.r
#define IR      Z->ir.r

#define AF_     Z->af_.r
#define BC_     Z->bc_.r
#define DE_     Z->de_.r
#define HL_     Z->hl_.r

#define MP      Z->m.r

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

// Flags
#define F_CARRY     0x01
#define F_NEG       0x02
#define F_PARITY    0x04
#define F_3         0x08
#define F_HALF      0x10
#define F_5         0x20
#define F_ZERO      0x40
#define F_SIGN      0x80

// These tables generated everytime z80Init() is called.  It doesn't matter if you generate them multiple times since
// they never change.
u8 gParity[256];
u8 gSZ53[256];
u8 gSZ53P[256];

// Other constantly defined tables
u8 kIoIncParityTable[16] = { 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 };
u8 kIoDecParityTable[16] = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1 };
u8 kHalfCarryAdd[8] = { 0, F_HALF, F_HALF, F_HALF, 0, 0, 0, F_HALF };
u8 kHalfCarrySub[8] = { 0, 0, F_HALF, 0, F_HALF, 0, F_HALF, F_HALF };
u8 kOverflowAdd[8] = { 0, 0, 0, F_PARITY, F_PARITY, 0, 0, 0 };
u8 kOverflowSub[8] = { 0, F_PARITY, 0, 0, 0, 0, F_PARITY, 0 };

//----------------------------------------------------------------------------------------------------------------------
// Flag manipulation
//----------------------------------------------------------------------------------------------------------------------

void z80SetFlag(Z80* Z, u8 flags, bool value)
{
    if (value)
    {
        F |= flags;
    }
    else
    {
        F &= ~flags;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Initialisation
//----------------------------------------------------------------------------------------------------------------------

void z80Init(Z80* Z, Memory* mem)
{
    memoryClear(Z, sizeof(*Z));
    Z->mem = mem;

    for (int i = 0; i < 256; ++i)
    {
        gSZ53[i] = (u8)(i & (F_3 | F_5 | F_SIGN));

        u8 p = 0, x = i;
        for (int bit = 0; bit < 8; ++bit) { p ^= (u8)(x & 1); x >>= 1; }
        gParity[i] = p ? 0 : F_PARITY;

        gSZ53P[i] = gSZ53[i] | gParity[i];
    }

    gSZ53[0] |= F_ZERO;
    gSZ53P[0] |= F_ZERO;
}

//----------------------------------------------------------------------------------------------------------------------
// Instruction utilities
//----------------------------------------------------------------------------------------------------------------------

void z80Exx(Z80* Z)
{
    u16 t;

    t = BC;
    BC = BC_;
    BC_ = t;

    t = DE;
    DE = DE_;
    DE_ = t;

    t = HL;
    HL = HL_;
    HL_ = t;
}

void z80ExAfAf(Z80* Z)
{
    u16 t = AF;
    AF = AF_;
    AF_ = t;
}

void z80IncReg(Z80* Z, Ref* reg)
{
    *reg->w = (*reg->r + 1);

    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 3
    // P: Result is 0x80
    // N: Reset
    // C: Unaffected
    F = (F & F_CARRY) | ((*reg->r == 0x80) ? F_PARITY : 0) | ((*reg->r & 0x0f) ? 0 : F_HALF) | gSZ53[*reg->r];
}

void z80DecReg(Z80* Z, Ref* reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 4
    // P: Result is 0x7f
    // N: Set
    // C: Unaffected
    F = (F & F_CARRY) | ((*reg->r & 0x0f) ? 0 : F_HALF) | F_NEG;

    *reg->w = (*reg->r - 1);

    F |= (*reg->r == 0x7f ? F_PARITY : 0) | gSZ53[*reg->r];
}

void z80AddReg16(Z80* Z, u16* r1, u16* r2)
{
    u32 add = (u32)*r1 + (u32)*r2;

    // S: Not affected
    // Z: Not affected
    // H: Set if carry from bit 11
    // P: Not affected
    // N: Reset
    // C: Carry from bit 15
    u8 x = (u8)(((*r1 & 0x0800) >> 11) | ((*r2 & 0x0800) >> 10) | ((add & 0x0800) >> 9));
    F = (F & (F_PARITY | F_ZERO | F_SIGN)) | (add & 0x10000 ? F_CARRY : 0) | ((add >> 8) & (F_3 | F_5)) | kHalfCarryAdd[x];
    *r1 = (u16)add;
}

// Result always goes into A.
void z80AddReg8(Z80* Z, u8* r)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 3
    // P: Set if overflow
    // N: Reset
    // C: Carry from bit 7
    u16 t = A + *r;
    u8 x = (u8)(((A & 0x88) >> 3) | ((*r & 0x88) >> 2) | ((t & 0x88) >> 1));
    A = (u8)t;
    F = ((t & 0x100) ? F_CARRY : 0) | kHalfCarryAdd[x & 0x07] | kOverflowAdd[x >> 4] | gSZ53[A];
}

// Result always goes into HL
void z80AdcReg16(Z80* Z, u16* r)
{
    // S: Not affected
    // Z: Not affected
    // H: Set if carry from bit 11
    // P: Not affected
    // N: Reset
    // C: Carry from bit 15
    u32 t = (u32)HL + (u32)*r + (F & F_CARRY);
    u8 x = (u8)(((HL & 0x8800) >> 11) | ((*r & 0x8800) >> 10) | ((t & 0x8800) >> 9));
    HL = (u16)t;
    F = ((t & 0x10000) ? F_CARRY : 0) | kOverflowAdd[x >> 4] | (H & (F_3 | F_5 | F_SIGN)) | kHalfCarryAdd[x & 0x07] |
        (HL ? 0 : F_ZERO);
}

// Result always goes into A
void z80AdcReg8(Z80* Z, u8* r)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 3
    // P: Set if overflow
    // N: Reset
    // C: Carry from bit 7
    u16 t = (u16)A + *r + (F & F_CARRY);
    u8 x = (u8)(((A & 0x88) >> 3) | ((*r & 0x88) >> 2) | ((t & 0x88) >> 1));
    A = (u8)t;
    F = ((t & 0x100) ? F_CARRY : 0) | kHalfCarryAdd[x & 0x07] | kOverflowAdd[x >> 4] | gSZ53[A];
}

void z80SubReg8(Z80* Z, u8* r)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Borrow from bit 4
    // P: Set if overflow
    // N: Set
    // C: Set if borrowed
    u16 t = (u16)A - *r;
    u8 x = (u8)(((A & 0x88) >> 3) | ((*r & 0x88) >> 2) | ((t & 0x88) >> 1));
    A = (u8)t;
    F = ((t & 0x100) ? F_CARRY : 0) | F_NEG | kHalfCarrySub[x & 0x07] | kOverflowSub[x >> 4] | gSZ53[A];
}

void z80SbcReg8(Z80* Z, u8* r)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Borrow from bit 4
    // P: Set if overflow
    // N: Set
    // C: Set if borrowed
    u16 t = (u16)A - *r - (F & F_CARRY);
    u8 x = (u8)(((A & 0x88) >> 3) | ((*r & 0x88) >> 2) | ((t & 0x88) >> 1));
    A = (u8)t;
    F = ((t & 0x100) ? F_CARRY : 0) | F_NEG | kHalfCarrySub[x & 0x07] | kOverflowSub[x >> 4] | gSZ53[A];
}

void z80SbcReg16(Z80* Z, u16* r)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Borrow from bit 12
    // P: Set if overflow
    // N: Set
    // C: Set if borrowed
    u32 t = (u32)HL - *r - (F & F_CARRY);
    u8 x = (u8)(((HL & 0x8800) >> 11) | ((*r & 0x8800) >> 10) | ((t & 0x8800) >> 9));
    HL = (u16)t;
    F = ((t & 0x10000) ? F_CARRY : 0) | F_NEG | kOverflowSub[x >> 4] | (H & (F_3 | F_5 | F_SIGN))
        | kHalfCarrySub[x & 0x07] | (HL ? 0 : F_ZERO);
}

void z80CpReg(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Borrow from 4 during 'subtraction'
    // P: Overflow (r > A)
    // N: Set
    // C: Set if borrowed (r > A)
    int t = (int)A - *r;
    u8 x = (u8)(((A & 0x88) >> 3) | ((*r & 0x88) >> 2) | ((t & 0x88) >> 1));
    F = ((t & 0x100) ? F_CARRY : (t ? 0 : F_ZERO)) | F_NEG | kHalfCarrySub[x & 7] | kOverflowSub[x >> 4] |
        (*r & (F_3 | F_5)) | (t & F_SIGN);
}

void z80AndReg8(Z80* Z, u8* r)
{
    A &= *r;

    // S, Z: Based on result
    // H: Set
    // P: Overflow
    // N: Reset
    // C: Reset
    F = F_HALF | gSZ53P[A];
}

void z80OrReg8(Z80* Z, u8* r)
{
    A |= *r;

    // S, Z: Based on result
    // H: Set
    // P: Overflow
    // N: Reset
    // C: Reset
    F = F_HALF | gSZ53P[A];
}

void z80XorReg8(Z80* Z, u8* r)
{
    A ^= *r;

    // S, Z: Based on result
    // H: Set
    // P: Overflow
    // N: Reset
    // C: Reset
    F = F_HALF | gSZ53P[A];
}

//         +-------------------------------------+
//  +---+  |  +---+---+---+---+---+---+---+---+  |
//  | C |<-+--| 7                           0 |<-+
//  +---+     +---+---+---+---+---+---+---+---+
//
void z80RlcReg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    *r->w = ((*r->r << 1) | (*r->r >> 7));
    F = (*r->r & F_CARRY) | gSZ53P[*r->r];
}

//  +-------------------------------------+
//  |  +---+---+---+---+---+---+---+---+  |  +---+
//  +->| 7                           0 |--+->| C |
//     +---+---+---+---+---+---+---+---+     +---+
//
void z80RrcReg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F = *r->r & F_CARRY;
    *r->w = ((*r->r >> 1) | (*r->r << 7));
    F |= gSZ53P[*r->r];
}

//  +-----------------------------------------------+
//  |  +---+     +---+---+---+---+---+---+---+---+  |
//  +--| C |<----| 7                           0 |<-+
//     +---+     +---+---+---+---+---+---+---+---+
//
void z80RlReg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    u8 t = *r->r;
    *r->w = ((*r->r << 1) | (F & F_CARRY));
    F = (t >> 7) | gSZ53P[*r->r];
}

//  +-----------------------------------------------+
//  |  +---+---+---+---+---+---+---+---+     +---+  |
//  +->| 7                           0 |---->| C |--+
//     +---+---+---+---+---+---+---+---+     +---+
//
void z80RrReg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    u8 t = *r->r;
    *r->w = ((*r->r >> 1) | (F << 7));
    F = (t & F_CARRY) | gSZ53P[*r->r];
}

//  +---+     +---+---+---+---+---+---+---+---+
//  | C |<----| 7                           0 |<---- 0
//  +---+     +---+---+---+---+---+---+---+---+
//
void z80SlaReg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    F = *r->r >> 7;
    *r->w = (*r->r << 1);
    F |= gSZ53P[*r->r];
}

//     +---+---+---+---+---+---+---+---+     +---+
//  +--| 7                           0 |---->| C |
//  |  +---+---+---+---+---+---+---+---+     +---+
//  |    ^
//  |    |
//  +----+
//
void z80SraReg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F = *r->r & F_CARRY;
    *r->w = ((*r->r & 0x80) | (*r->r >> 1));
    F |= gSZ53P[*r->r];
}

//  +---+     +---+---+---+---+---+---+---+---+
//  | C |<----| 7                           0 |<---- 1
//  +---+     +---+---+---+---+---+---+---+---+
//
void z80Sl1Reg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    F = *r->r >> 7;
    *r->w = ((*r->r << 1) | 0x01);
    F |= gSZ53P[*r->r];
}

//         +---+---+---+---+---+---+---+---+     +---+
//  0 ---->| 7                           0 |---->| C |
//         +---+---+---+---+---+---+---+---+     +---+
//
void z80SrlReg8(Z80* Z, Ref* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F = *r->r & F_CARRY;
    *r->w = (*r->r >> 1);
    F |= gSZ53P[*r->r];
}

void z80BitReg8(Z80* Z, u8* r, int b)
{
    // S: Undefined (set to bit 7 if bit 7 is checked, otherwise 0)
    // Z: Opposite of bit b
    // H: Set
    // P: Undefined (same as Z)
    // N: Reset
    // C: Preserved
    F = (F & F_CARRY) | F_HALF | (*r & (F_3 | F_5));
    if (!(*r & (1 << b))) F |= F_PARITY | F_ZERO;
    if ((b == 7) && (*r & 0x80)) F |= F_SIGN;
}

void z80ResReg8(Z80* Z, Ref* r, int b)
{
    // All flags preserved.
    *r->w = *r->r & ~((u8)1 << b);
}

void z80SetReg8(Z80* Z, Ref* r, int b)
{
    // All flags preserved.
    *r->w = *r->r | ((u8)1 << b);
}

void z80Daa(Z80* Z)
{
    u8 result = A;
    u8 incr = 0;
    bool carry = K_BOOL(F & F_CARRY);

    if (((F & F_HALF) != 0) || ((result & 0x0f) > 0x09))
    {
        incr |= 0x06;
    }

    if (carry || (result > 0x9f) || ((result > 0x8f) && ((result & 0x0f) > 0x09)))
    {
        incr |= 0x60;
    }

    if (result > 0x99) carry = YES;

    if ((F & F_NEG) != 0)
    {
        z80SubReg8(Z, &incr);
    }
    else
    {
        z80AddReg8(Z, &incr);
    }

    result = A;

    z80SetFlag(Z, F_CARRY, carry);
    z80SetFlag(Z, F_PARITY, gParity[result]);
}

int z80Displacement(u8 x)
{
    return (128 ^ (int)x) - 128;
}

u16 z80Pop(Z80* Z, i64* tState)
{
    u16 x = memoryPeek16(Z->mem, SP, tState);
    SP += 2;
    return x;
}

void z80Push(Z80* Z, u16 x, i64* tState)
{
    SP -= 2;
    memoryPoke16(Z->mem, SP, x, tState);
}

//----------------------------------------------------------------------------------------------------------------------
// z80Step
// Run a single instruction
//----------------------------------------------------------------------------------------------------------------------

Ref z80GetReg(Z80* Z, u8 y, i64* tState)
{
    Ref r;
    switch (y)
    {
    case 0:     r.r = &B;   r.w = &B;   break;
    case 1:     r.r = &C;   r.w = &C;   break;
    case 2:     r.r = &D;   r.w = &D;   break;
    case 3:     r.r = &E;   r.w = &E;   break;
    case 4:     r.r = &H;   r.w = &H;   break;
    case 5:     r.r = &L;   r.w = &L;   break;
    case 7:     r.r = &A;   r.w = &A;   break;

    case 6:
        r = memoryRef(Z->mem, HL, tState);
    }

    return r;
}

u16* z80GetReg16_1(Z80* Z, u8 p)
{
    switch (p)
    {
    case 0: return &BC;
    case 1: return &DE;
    case 2: return &HL;
    case 3: return &SP;
    }

    return 0;
}

u16* z80GetReg16_2(Z80* Z, u8 p)
{
    switch (p)
    {
    case 0: return &BC;
    case 1: return &DE;
    case 2: return &HL;
    case 3: return &AF;
    }

    return 0;
}

bool z80GetFlag(u8 y, u8 flags)
{
    switch (y)
    {
    case 0: return K_BOOL(!(flags & F_ZERO));
    case 1: return K_BOOL(flags & F_ZERO);
    case 2: return K_BOOL(!(flags & F_CARRY));
    case 3: return K_BOOL(flags & F_CARRY);
    case 4: return K_BOOL(!(flags & F_PARITY));
    case 5: return K_BOOL(flags & F_PARITY);
    case 6: return K_BOOL(!(flags & F_SIGN));
    case 7: return K_BOOL(flags & F_SIGN);
    }

    return YES;
}

typedef void(*ALUFunc) (Z80* Z, u8* r);

ALUFunc z80GetAlu(u8 y)
{
    static ALUFunc funcs[8] =
    {
        &z80AddReg8,
        &z80AdcReg8,
        &z80SubReg8,
        &z80SbcReg8,
        &z80AndReg8,
        &z80XorReg8,
        &z80OrReg8,
        &z80AndReg8
    };

    return funcs[y];
};

typedef void(*RotShiftFunc) (Z80* Z, Ref* r);

RotShiftFunc z80GetRotateShift(u8 y)
{
    static RotShiftFunc funcs[8] =
    {
        &z80RlcReg8,
        &z80RrcReg8,
        &z80RlReg8,
        &z80RrReg8,
        &z80SlaReg8,
        &z80SraReg8,
        &z80Sl1Reg8,
        &z80SrlReg8,
    };

    return funcs[y];
}


#define PEEK(a) memoryPeek(Z->mem, (a), tState)
#define POKE(a, b) memoryPoke(Z->mem, (a), (b), tState)
#define PEEK16(a) memoryPeek16(Z->mem, (a), tState)
#define POKE16(a, w) memoryPoke16(Z->mem, (a), (w), tState)
#define CONTEND(a, t, n) memoryContend(Z->mem, (a), (t), (n), tState)

// Temporary place holders
#define ioIn(io, port) ((u8)0xff)
#define ioOut(io, port, b) do { } while(0)

u8 z80FetchInstruction(Z80* Z, u8* x, u8* y, u8* z, u8* p, u8* q, i64* tState)
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
    ++R;
    CONTEND(PC, 4, 1);
#if NX_RUN_TESTS
    u8 opCode = memoryPeekNoContend(Z->mem, PC++, *tState);
#else
    u8 opCode = memoryPeekNoContend(Z->mem, PC++);
#endif
    *x = (opCode & 0xc0) >> 6;
    *y = (opCode & 0x38) >> 3;
    *z = (opCode & 0x07);
    *p = (*y & 6) >> 1;
    *q = (*y & 1);
    return opCode;
}

void z80Step(Z80* Z, i64* tState)
{
    u8 x, y, z, p, q;
    u8 opCode = z80FetchInstruction(Z, &x, &y, &z, &p, &q, tState);

    // Commonly used local variables
    u8 d = 0;       // Used for displacement
    u16 tt = 0;     // Temporary for an address
    Ref r;          // Temporary for a reference

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
                z80ExAfAf(Z);
                break;

            case 2:     // 10 - DJNZ d
                CONTEND(IR, 1, 1);
                --B;
                if (B != 0)
                {
                    d = z80Displacement(PEEK(PC));
                    CONTEND(PC, 1, 5);
                    PC += (i8)(d + 1);
                    MP = PC;
                }
                else
                {
                    CONTEND(PC, 3, 1);
                    ++PC;
                }
                break;

            case 3:     // 18 - JR d
                d = z80Displacement(PEEK(PC));
                CONTEND(PC, 1, 5);
                PC += (i8)(d + 1);
                MP = PC;
                break;

            default:    // 20, 28, 30, 38 - JR cc(y-4),d
                if (z80GetFlag(y - 4, F))
                {
                    d = z80Displacement(PEEK(PC));
                    CONTEND(PC, 1, 5);
                    PC += (i8)(d + 1);
                    MP = PC;
                }
                else
                {
                    CONTEND(PC, 3, 1);
                    ++PC;
                }
            }
            break;

        case 1: // x, z = (0, 1)
            if (0 == q)
            {
                // 01, 11, 21, 31 - LD BC/DE/HL/SP, nnnn
                u16* rr = z80GetReg16_1(Z, p);
                *rr = PEEK16(PC);
                PC += 2;
            }
            else
            {
                // 09, 19, 29, 39 - ADD HL, BC/DE/HL/SP
                CONTEND(IR, 1, 7);
                MP = HL + 1;
                z80AddReg16(Z, &HL, z80GetReg16_1(Z, p));
            }
            break;

        case 2: // x, z = (0, 2)
            switch (y)
            {
            case 0:     // 02 - LD (BC),A
                POKE(BC, A);
                MP = (((BC + 1) & 0xff) | (A << 8));
                break;

            case 1:     // 0A - LD A,(BC)
                A = PEEK(BC);
                MP = BC + 1;
                break;

            case 2:     // 12 - LD (DE),A
                POKE(DE, A);
                MP = (((DE + 1) & 0xff) | (A << 8));
                break;

            case 3:     // 1A - LD A,(DE)
                A = PEEK(DE);
                MP = DE + 1;
                break;

            case 4:     // 22 - LD (nn),HL
                tt = PEEK16(PC);
                POKE16(tt, HL);
                MP = tt + 1;
                PC += 2;
                break;

            case 5:     // 2A - LD HL,(nn)
                tt = PEEK16(PC);
                HL = PEEK16(tt);
                PC += 2;
                MP = tt + 1;
                break;

            case 6:     // 32 - LD (nn),A
                tt = PEEK16(PC);
                POKE(tt, A);
                MP = (((tt + 1) & 0xff) | (A << 8));
                PC += 2;
                break;

            case 7:     // 3A - LD A,(nn)
                tt = PEEK16(PC);
                MP = tt + 1;
                A = PEEK(tt);
                PC += 2;
                break;
            }
            break;

        case 3: // x, z = (0, 3)
            if (0 == q)
            {
                // 03, 13, 23, 33 - INC BC/DE/HL/SP
                CONTEND(IR, 1, 2);
                ++(*z80GetReg16_1(Z, p));
            }
            else
            {
                // 0B, 1B, 2B, 3B - DEC BC/DE/HL/SP
                CONTEND(IR, 1, 2);
                --(*z80GetReg16_1(Z, p));
            }
            break;

        case 4: // x, z = (0, 4)
            // 04, 0C, 14, 1C, 24, 2C, 34, 3C - INC B/C/D/E/H/L/(HL)/A
            r = z80GetReg(Z, y, tState);
            z80IncReg(Z, &r);
            break;

        case 5: // x, z = (0, 5)
            // 05, 0D, 15, 1D, 25, 2D, 35, 3D - DEC B/C/D/E/H/L/(HL)/A
            r = z80GetReg(Z, y, tState);
            z80DecReg(Z, &r);
            break;

        case 6: // x, z = (0, 6)
            // 06, 0E, 16, 1E, 26, 2E, 36, 3E - LD B/C/D/E/H/L/(HL)/A, n
            r = z80GetReg(Z, y, tState);
            *r.w = PEEK(PC++);
            break;

        case 7: // x, z = (0, 7)
            switch (y)
            {
            case 0:     // 07 - RLCA
                A = ((A << 1) | (A >> 7));
                F = (F & (F_PARITY | F_ZERO | F_SIGN)) | (A & (F_CARRY | F_3 | F_5));
                break;

            case 1:     // 0F - RRCA
                F = (F & (F_PARITY | F_ZERO | F_SIGN)) | (A & F_CARRY);
                A = ((A >> 1) | (A << 7));
                F |= (A & (F_3 | F_5));
                break;

            case 2:     // 17 - RLA
                d = A;
                A = (A << 1) | (F & F_CARRY);
                F = (F & (F_PARITY | F_ZERO | F_SIGN)) | (A & (F_3 | F_5)) | (d >> 7);
                break;

            case 3:     // 1F - RRA
                d = A;
                A = (A >> 1) | (F << 7);
                F = (F & (F_PARITY | F_ZERO | F_SIGN)) | (A & (F_3 | F_5)) | (d & F_CARRY);
                break;

            case 4:     // 27 - DAA
                z80Daa(Z);
                break;

            case 5:     // 2F - CPL
                A = A ^ 0xff;
                F = (F & (F_CARRY | F_PARITY | F_ZERO | F_SIGN)) | (A & (F_3 | F_5)) | F_NEG | F_HALF;
                break;

            case 6:     // 37 - SCF
                F = (F & (F_PARITY | F_ZERO | F_SIGN)) | (A & (F_3 | F_5)) | F_CARRY;
                break;

            case 7:     // 3F - CCF
                F = (F & (F_PARITY | F_ZERO | F_SIGN)) | (A & (F_3 | F_5)) | ((F & F_CARRY) ? F_HALF : F_CARRY);
                break;
            }
            break;

        } // switch(z)
        break; // x == 0

    case 1:
        if (z == 6 && y == 6)
        {
            // 76 - HALT
            Z->halt = YES;
            --PC;
        }
        else
        {
            // 40 - 7F - LD R,R
            Ref r1 = z80GetReg(Z, y, tState);
            Ref r2 = z80GetReg(Z, z, tState);
            *r1.w = *r2.r;
        }
        break; // x == 1

    case 2:
        {
            r = z80GetReg(Z, z, tState);
            z80GetAlu(y)(Z, r.r);
        }
        break; // x == 2

    case 3:
        switch (z)
        {
        case 0:
            // C0, C8, D0, D8, E0, E8, F0, F8 - RET flag
            CONTEND(IR, 1, 1);
            if (z80GetFlag(y, F))
            {
                PC = z80Pop(Z, tState);
                MP = PC;
            }
            break;  // x, z = (3, 0)

        case 1:
            if (0 == q)
            {
                // C1, D1, E1, F1 - POP RR
                *z80GetReg16_2(Z, p) = z80Pop(Z, tState);
            }
            else
            {
                switch (p)
                {
                case 0:     // C9 - RET
                    PC = z80Pop(Z, tState);
                    MP = PC;
                    break;

                case 1:     // D9 - EXX
                    z80Exx(Z);
                    break;

                case 2:     // E9 - JP HL
                    PC = HL;
                    break;

                case 3:     // F9 - LD SP, HL
                    CONTEND(IR, 1, 1);
                    SP = HL;
                    break;
                }
            }
            break;  // x, z = (3, 1)

        case 2:
            // C2, CA, D2, DA, E2, EA, F2, FA - JP flag,nn
            tt = PEEK16(PC);
            if (z80GetFlag(y, F))
            {
                PC = tt;
            }
            else
            {
                PC += 2;
            }
            MP = tt;
            break;  // x, z = (3, 2)

        case 3:
            switch (y)
            {
            case 0:     // C3 - JP nn
                PC = PEEK16(PC);
                MP = PC;
                break;

            case 1:     // CB (prefix)
                opCode = z80FetchInstruction(Z, &x, &y, &z, &p, &q, tState);
                switch (x)
                {
                case 0:     // 00-3F: Rotate/Shift instructions
                    r = z80GetReg(Z, z, tState);
                    z80GetRotateShift(y)(Z, &r);
                    break;

                case 1:     // 40-7F: BIT instructions
                    r = z80GetReg(Z, z, tState);
                    z80BitReg8(Z, r.r, y);
                    break;

                case 2:     // 80-BF: RES instructions
                    r = z80GetReg(Z, z, tState);
                    z80ResReg8(Z, &r, y);
                    break;

                case 3:     // C0-FF: SET instructions
                    r = z80GetReg(Z, z, tState);
                    z80SetReg8(Z, &r, y);
                }
                break;

            case 2:     // D3 - OUT (n),A       A -> $AAnn
                // #todo: I/O
                d = PEEK(PC);
                ioOut(Z->io, (u16)d | ((u16)A << 8), A);
                MP = ((u16)(d + 1)) | ((u16)A << 8);
                ++PC;
                break;

            case 3:     // DB - IN A,(n)        A <- $AAnn
                d = PEEK(PC);
                tt = ((u16)A << 8) | d;
                MP = ((u16)A << 8) + d + 1;
                A = ioIn(Z->io, tt);
                ++PC;
                break;

            case 4:     // E3 - EX (SP),HL
                tt = PEEK16(SP);
                CONTEND(SP + 1, 1, 1);
                POKE16(SP, HL);
                CONTEND(SP, 1, 2);
                HL = tt;
                MP = HL;
                break;

            case 5:     // EB - EX DE,HL
                tt = DE;
                DE = HL;
                HL = tt;
                break;

            case 6:     // F3 - DI
                Z->iff1 = NO;
                Z->iff2 = NO;
                break;

            case 7:     // FB - EI
                Z->iff1 = YES;
                Z->iff2 = YES;
                Z->lastOpcodeWasEI = YES;
                break;
            }
            break;  // x, z = (3, 3)

        case 4:
            // C4 CC D4 DC E4 EC F4 FC - CALL F,nn
            tt = PEEK16(PC);
            MP = tt;
            if (z80GetFlag(y, F))
            {
                CONTEND(PC + 1, 1, 1);
                z80Push(Z, PC + 2, tState);
                PC = tt;
            }
            else
            {
                PC += 2;
            }
            break;  // x, z = (3, 4)

        case 5:
            if (0 == q)
            {
                // C5 D5 E5 F5 - PUSH RR
                CONTEND(IR, 1, 1);
                z80Push(Z, *z80GetReg16_2(Z, p), tState);
            }
            else
            {
                switch (p)
                {
                case 0:     // CD - CALL nn
                    tt = PEEK16(PC);
                    MP = tt;
                    CONTEND(PC + 1, 1, 1);
                    z80Push(Z, PC + 2, tState);
                    PC = tt;
                    break;

                case 1:     // DD - IX prefix
                    break;

                case 2:     // ED - extensions prefix
                    break;

                case 3:     // FD - IY prefix
                    break;
                }
            }
            break;  // x, z = (3, 5)

        case 6:
            // C6, CE, D6, DE, E6, EE, F6, FE - ALU A,n
            d = PEEK(PC++);
            z80GetAlu(y)(Z, &d);
            break;  // x, z = (3, 6)

        case 7:
            // C7, CF, D7, DF, E7, EF, F7, FF - RST n
            CONTEND(IR, 1, 1);
            z80Push(Z, PC, tState);
            PC = (u16)y * 8;
            MP = PC;
            break;  // x, z = (3, 7)

        }
        break; // x == 3
    } // switch(x)
}

#undef PEEK
#undef POKE
#undef PEEK16
#undef POKE16
#undef CONTEND

#undef F_CARRY
#undef F_NEG
#undef F_PARITY
#undef F_3
#undef F_HALF
#undef F_5
#undef F_ZERO
#undef F_SIGN

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
