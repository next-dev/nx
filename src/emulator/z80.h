//----------------------------------------------------------------------------------------------------------------------
// Z80 emulation
//----------------------------------------------------------------------------------------------------------------------

#include <core.h>

#include <functional>

//----------------------------------------------------------------------------------------------------------------------
// CPU interface to external systems
//----------------------------------------------------------------------------------------------------------------------

struct IExternals
{
    // Memory access
    virtual u8 peek(u16 address) = 0;
    virtual u8 peek(u16 address, TState& t) = 0;
    virtual u16 peek16(u16 address, TState& t) = 0;
    virtual void poke(u16 address, u8 x, TState& t) = 0;
    virtual void poke16(u16 address, u16 x, TState& t) = 0;

    // Contention
    virtual void contend(u16 address, TState delay, int num, TState& t) = 0;

    // I/O
    virtual u8 in(u16 port, TState& t) = 0;
    virtual void out(u16 port, u8 x, TState& t) = 0;
};

//----------------------------------------------------------------------------------------------------------------------
// Z80 emulation
//----------------------------------------------------------------------------------------------------------------------

class Z80
{
public:

    Z80(IExternals& ext);

    void step(TState& tState);
    void interrupt();
    void nmi();
    void restart();

    bool isHalted() const { return m_halt; }

    u8& A() { return m_af.h; }
    u8& F() { return m_af.l; }
    u8& B() { return m_bc.h; }
    u8& C() { return m_bc.l; }
    u8& D() { return m_de.h; }
    u8& E() { return m_de.l; }
    u8& H() { return m_hl.h; }
    u8& L() { return m_hl.l; }
    u8& IXH() { return m_ix.h; }
    u8& IXL() { return m_ix.l; }
    u8& IYH() { return m_iy.h; }
    u8& IYL() { return m_iy.l; }
    u8& I() { return m_ir.h; }
    u8& R() { return m_ir.l; }

    u16& AF() { return m_af.r; }
    u16& BC() { return m_bc.r; }
    u16& DE() { return m_de.r; }
    u16& HL() { return m_hl.r; }
    u16& IX() { return m_ix.r; }
    u16& IY() { return m_iy.r; }
    u16& SP() { return m_sp.r; }
    u16& PC() { return m_pc.r; }
    u16& IR() { return m_ir.r; }

    u16& AF_() { return m_af_.r; }
    u16& BC_() { return m_bc_.r; }
    u16& DE_() { return m_de_.r; }
    u16& HL_() { return m_hl_.r; }

    u16& MP() { return m_mp.r; }

    bool& IFF1() { return m_iff1; }
    bool& IFF2() { return m_iff2; }
    int& IM() { return m_im; }

    static const u8 F_CARRY = 0x01;
    static const u8 F_NEG = 0x02;
    static const u8 F_PARITY = 0x04;
    static const u8 F_3 = 0x08;
    static const u8 F_HALF = 0x10;
    static const u8 F_5 = 0x20;
    static const u8 F_ZERO = 0x40;
    static const u8 F_SIGN = 0x80;

    // Pop is public because it is needed for snapshot loading
    u16 pop(TState& inOutTState);
    void push(u16 x, TState& inOutTState);


private:
    void setFlags(u8 flags, bool value);

    void exx();
    void exAfAf();

    void incReg8(u8& reg);
    void decReg8(u8& reg);
    void addReg16(u16& r1, u16& r2);
    void addReg8(u8& reg);
    void adcReg16(u16& reg);
    void adcReg8(u8& reg);
    void subReg8(u8& reg);
    void sbcReg8(u8& reg);
    void sbcReg16(u16& reg);
    void cpReg8(u8& reg);
    void andReg8(u8& reg);
    void orReg8(u8& reg);
    void xorReg8(u8& reg);
    void rlcReg8(u8& reg);
    void rrcReg8(u8& reg);
    void rlReg8(u8& reg);
    void rrReg8(u8& reg);
    void slaReg8(u8& reg);
    void sraReg8(u8& reg);
    void sl1Reg8(u8& reg);
    void srlReg8(u8& reg);
    void bitReg8(u8& reg, int b);
    void bitReg8MP(u8& reg, int b);
    void resReg8(u8& reg, int b);
    void setReg8(u8& reg, int b);
    void daa();
    int displacement(u8 x);

    u8& getReg8(u8 y);
    u16& getReg16_1(u8 p);
    u16& getReg16_2(u8 p);
    bool getFlag(u8 y, u8 flags);

    using ALUFunc = function<void(u8&)>;
    using RotShiftFunc = function<void(u8&)>;

    ALUFunc getAlu(u8 y);
    RotShiftFunc getRotateShift(u8 y);

    void decodeInstruction(u8 opCode, u8& x, u8& y, u8& z, u8& p, u8& q);
    u8 fetchInstruction(TState& tState);

    void execute(u8 opCode, TState& tState);
    void executeDDFDCB(Reg& idx, TState& tState);
    void executeDDFD(Reg& idx, TState& tState);
    void executeED(TState& tState);


private:
    IExternals& m_ext;

    // Base registers
    Reg         m_af, m_bc, m_de, m_hl;
    Reg         m_sp, m_pc, m_ix, m_iy;
    Reg         m_ir;

    // Alternate registers
    Reg         m_af_, m_bc_, m_de_, m_hl_;

    // Internal registers
    Reg         m_mp;

    bool        m_halt;
    bool        m_iff1;
    bool        m_iff2;
    int         m_im;
    bool        m_interrupt;    // Set to false when interrupt is supposed to be triggered.
    bool        m_nmi;          // Set to false when nmi occurs.
    bool        m_eiHappened;   // Set to false when EI is called.  This stops the interrupt occurring for at least one
                                // instruction afterwards.

    u8          m_parity[256];
    u8          m_SZ53[256];
    u8          m_SZ53P[256];

    bool        m_flagsChanged;
    bool        m_lastFlagsChanged;

    static const u8 kIoIncParityTable[16];
    static const u8 kIoDecParityTable[16];
    static const u8 kHalfCarryAdd[8];
    static const u8 kHalfCarrySub[8];
    static const u8 kOverflowAdd[8];
    static const u8 kOverflowSub[8];
};

