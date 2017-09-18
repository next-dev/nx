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
    // Base registers
    Reg     af, bc, de, hl;
    Reg     sp, pc, ix, iy;
    Reg     ir;

    // Alternate registers
    Reg     af_, bc_, de_, hl_;

    // Internal registers
    Reg     m;
}
Z80;

// Initialise the internal tables and data structure.
void z80Init(Z80* Z);

// Run a single opcode
void z80Run(Z80* Z);

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

#define AF_     Z->af_.r
#define BC_     Z->bc_.r
#define DE_     Z->de_.r
#define HL_     Z->hl_.r

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

void z80Init(Z80* Z)
{
    memoryClear(Z, sizeof(*Z));

    for (int i = 0; i < 256; ++i)
    {
        gSZ53[i] = (u8)(i & (F_3 | F_5 | F_SIGN));

        u8 p = 0, x = i;
        for (int b = 0; b < 8; ++b) { p ^= (u8)(b & 1); b >>= 1; }
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

void z80IncReg(Z80* Z, u8* reg)
{
    *reg = (*reg + 1);

    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 3
    // P: Result is 0x80
    // N: Reset
    // C: Unaffected
    F = (F & F_CARRY) | ((*reg == 0x80) ? F_PARITY : 0) | ((*reg & 0x0f) ? 0 : F_HALF) | gSZ53[*reg];
}

void z80DecReg(Z80* Z, u8* reg)
{
    // S: Result is negative
    // Z: Result is zero
    // H: Carry from bit 4
    // P: Result is 0x7f
    // N: Set
    // C: Unaffected
    F = (F & F_CARRY) | ((*reg & 0x0f) ? 0 : F_HALF) | F_NEG;

    *reg = (*reg - 1);

    F |= (*reg == 0x7f ? F_PARITY : 0) | gSZ53[*reg];
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
void z80AdcReg(Z80* Z, u8* r)
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
void z80RlcReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    *r = ((*r << 1) | (*r >> 7));
    F = (*r & F_CARRY) | gSZ53P[*r];
}

//  +-------------------------------------+
//  |  +---+---+---+---+---+---+---+---+  |  +---+
//  +->| 7                           0 |--+->| C |
//     +---+---+---+---+---+---+---+---+     +---+
//
void z80RrcReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F = *r & F_CARRY;
    *r = ((*r >> 1) | (*r << 7));
    F |= gSZ53P[*r];
}

//  +-----------------------------------------------+
//  |  +---+     +---+---+---+---+---+---+---+---+  |
//  +--| C |<----| 7                           0 |<-+
//     +---+     +---+---+---+---+---+---+---+---+
//
void z80RlReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    u8 t = *r;
    *r = ((*r << 1) | (F & F_CARRY));
    F = (t >> 7) | gSZ53P[*r];
}

//  +-----------------------------------------------+
//  |  +---+---+---+---+---+---+---+---+     +---+  |
//  +->| 7                           0 |---->| C |--+
//     +---+---+---+---+---+---+---+---+     +---+
//
void z80RrReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    u8 t = *r;
    *r = ((*r >> 1) | (F << 7));
    F = (t & F_CARRY) | gSZ53P[*r];
}

//  +---+     +---+---+---+---+---+---+---+---+
//  | C |<----| 7                           0 |<---- 0
//  +---+     +---+---+---+---+---+---+---+---+
//
void z80SlaReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    F = *r >> 7;
    *r = (*r << 1);
    F |= gSZ53P[*r];
}

//     +---+---+---+---+---+---+---+---+     +---+
//  +--| 7                           0 |---->| C |
//  |  +---+---+---+---+---+---+---+---+     +---+
//  |    ^
//  |    |
//  +----+
//
void z80SraReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F = *r & F_CARRY;
    *r = ((*r & 0x80) | (*r >> 1));
    F |= gSZ53P[*r];
}

//  +---+     +---+---+---+---+---+---+---+---+
//  | C |<----| 7                           0 |<---- 1
//  +---+     +---+---+---+---+---+---+---+---+
//
void z80SllReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 7
    F = *r >> 7;
    *r = ((*r << 1) | 0x01);
    F |= gSZ53P[*r];
}

//         +---+---+---+---+---+---+---+---+     +---+
//  0 ---->| 7                           0 |---->| C |
//         +---+---+---+---+---+---+---+---+     +---+
//
void z80SrlReg8(Z80* Z, u8* r)
{
    // S, Z: Based on result
    // H: Reset
    // P: Set on even parity
    // N: Reset
    // C: bit 0
    F = *r & F_CARRY;
    *r = (*r >> 1);
    F |= gSZ53P[*r];
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

void z80ResReg8(Z80* Z, u8* r, int b)
{
    // All flags preserved.
    *r = *r & ~((u8)1 << b);
}

void z80SetReg8(Z80* Z, u8* r, int b)
{
    // All flags preserved.
    *r = *r | ((u8)1 << b);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
