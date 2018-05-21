//----------------------------------------------------------------------------------------------------------------------
// Base class for all models of Spectrum
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <audio/audio.h>
#include <config.h>
#include <emulator/z80.h>
#include <types.h>

#include <SFML/Graphics.hpp>

#include <array>
#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
// MemAddr
// Represents a bank and offset memory address
//----------------------------------------------------------------------------------------------------------------------

static const int kBankSize = KB(8);
static const int kNumBanks = 8;

enum class MemGroup
{
    RAM,        // Normal RAM
    ROM,        // Read only - cannot be written to
};

class Bank
{
public:
    Bank() : m_group(MemGroup::RAM), m_bank(0) {}
    Bank(MemGroup group, u16 bank);
    Bank(const Bank& other);
    Bank& operator= (const Bank& other);

    MemGroup getGroup() const { return m_group; }
    u16 getIndex() const { return m_bank; }

    bool operator== (const Bank& bank) const    { return m_group == bank.m_group && m_bank == bank.m_bank; }
    bool operator!= (const Bank& bank) const    { return m_group != bank.m_group || m_bank != bank.m_bank; }
    bool operator< (const Bank& bank) const     { assert(m_group == bank.m_group); return m_bank < bank.m_bank; }
    bool operator> (const Bank& bank) const     { assert(m_group == bank.m_group); return m_bank > bank.m_bank; }
    bool operator<= (const Bank& bank) const    { assert(m_group == bank.m_group); return m_bank <= bank.m_bank; }
    bool operator>= (const Bank& bank) const    { assert(m_group == bank.m_group); return m_bank >= bank.m_bank; }

private:
    MemGroup    m_group;
    u16         m_bank;
};

//
// MemAddr
//
// This represents a universal address that can address any piece of memory in the system.  It consists of a bank
// (which further consists of a memory group and bank index) and an offset.
//

class Z80MemAddr;

class MemAddr
{
public:
    // Default points to first RAM address.
    MemAddr() : m_bank(), m_offset(0) {}

    MemAddr(Bank bank, u16 offset);
    MemAddr(MemGroup group, int realAddress);

    MemAddr(const MemAddr& memAddr);
    MemAddr& operator= (const MemAddr& memAddr);

    Bank bank() const       { return m_bank; }
    u16 offset() const      { return m_offset;}

    int index() const { return m_bank.getIndex() * kBankSize + m_offset; }

    bool operator== (const MemAddr& addr) const;
    bool operator!= (const MemAddr& addr) const;
    bool operator< (const MemAddr& addr) const;
    bool operator> (const MemAddr& addr) const;
    bool operator<= (const MemAddr& addr) const;
    bool operator>= (const MemAddr& addr) const;

    MemAddr operator+ (int offset) const;
    MemAddr operator- (int offset) const;
    int operator- (MemAddr addr) const;

    MemAddr operator++();
    MemAddr& operator++(int);
    MemAddr operator--();
    MemAddr& operator--(int);

private:
    Bank        m_bank;
    u16         m_offset;
};

//----------------------------------------------------------------------------------------------------------------------
// Model
//----------------------------------------------------------------------------------------------------------------------

enum class Model
{
    ZX48,
    ZX128,
    ZXPlus2,
    //ZXPlus2A,
    //ZXPlus3,
    ZXNext,

    COUNT
};


//----------------------------------------------------------------------------------------------------------------------
// Configuration structure
//----------------------------------------------------------------------------------------------------------------------

struct SpectrumConfig
{
    u32*        image;
};

//----------------------------------------------------------------------------------------------------------------------
// Keyboard keys
//----------------------------------------------------------------------------------------------------------------------

enum class Key
{
    Shift, Z, X, C, V,
    A, S, D, F, G,
    Q, W, E, R, T,
    _1, _2, _3, _4, _5,
    _0, _9, _8, _7, _6,
    P, O, I, U, Y,
    Enter, L, K, J, H,
    Space, SymShift, M, N, B,
    
    COUNT
};

//----------------------------------------------------------------------------------------------------------------------
// Run mode
//----------------------------------------------------------------------------------------------------------------------

enum class RunMode
{
    Stopped,    // Don't run any instructions.
    Normal,     // Emulate as normal, run as fast as possible for a frame.
    StepIn,     // Step over a single instruction, and follow CALLs.
    StepOver,   // Step over a single instruction, and run a subroutine CALL till it returns to following instruction.
};

//----------------------------------------------------------------------------------------------------------------------
// Spectrum base class
// Each model must override this and implement the specifics
//----------------------------------------------------------------------------------------------------------------------

class Tape;

class Spectrum: public IExternals
{
public:
    // TState counter
    using TState        = i64;

    //------------------------------------------------------------------------------------------------------------------
    // Construction/Destruction
    //------------------------------------------------------------------------------------------------------------------

    Spectrum(function<void()> frameFunc);
    virtual ~Spectrum();

    //------------------------------------------------------------------------------------------------------------------
    // Control
    //------------------------------------------------------------------------------------------------------------------
    
    void            reset               (Model model);

    //------------------------------------------------------------------------------------------------------------------
    // State
    //------------------------------------------------------------------------------------------------------------------

    Model           getModel            () const { return m_model; }
    sf::Sprite&     getVideoSprite      ();
    TState          getFrameTime        () const { return 69888; }
    u8              getBorderColour     () const { return m_borderColour; }
    Z80&            getZ80              () { return m_z80; }
    TState          getTState           () { return m_tState;}
    Audio&          getAudio            () { return m_audio; }
    Tape*           getTape             () { return m_tape; }
    bool            isShadowScreen      () const { return m_shadowScreen; }
    bool            isPagingDisabled    () const { return m_pagingDisabled; }

    //------------------------------------------------------------------------------------------------------------------
    // IExternals interface
    //------------------------------------------------------------------------------------------------------------------

    u8              peek                (Z80MemAddr address) override;
    u8              peek                (Z80MemAddr address, TState& t) override;
    u16             peek16              (Z80MemAddr address, TState& t) override;
    void            poke                (Z80MemAddr address, u8 x, TState& t) override;
    void            poke16              (Z80MemAddr address, u16 x, TState& t) override;
    void            contend             (Z80MemAddr address, TState delay, int num, TState& t) override;
    u8              in                  (u16 port, TState& t) override;
    void            out                 (u16 port, u8 x, TState& t) override;

    //------------------------------------------------------------------------------------------------------------------
    // General functionality, not specific to a model
    //------------------------------------------------------------------------------------------------------------------

    // Main function called to generate a single frame or instruction (in single-step mode).  Will return true if
    // a frame was complete.
    bool            update              (RunMode runMode, bool& breakpointHit);

    // Emulation control
    void            togglePause         ();
    
    // Set the keyboard state.
    void            setKeyboardState    (vector<u8>& rows);
    
    // Set the border
    void            setBorderColour     (u8 borderColour);

    // Reset the tState counter
    void            resetTState         () { m_tState = 0; }

    // Set the tState counter
    void            setTState           (TState t) { m_tState = t;}

    // Set the tape, it will be played if not stopped.
    void            setTape             (Tape* tape) { m_tape = tape;}

    // Render all video, irregardless of t-state.
    void            renderVideo         ();

    //------------------------------------------------------------------------------------------------------------------
    // Memory interface
    //------------------------------------------------------------------------------------------------------------------

    int                 getRomSize          () const    { return 0x4000; }
    void                setSlot             (int slot, Bank bank);
    int                 getBank             (int slot) const;
    u16                 getNumBanks         () const;
    string&             slotName            (int slot);
    bool                isContended         (MemAddr addr) const;
    bool                isContended         (u16 port) const;
    TState              contention          (TState t);
    void                poke                (Z80MemAddr address, u8 x);
    void                load                (Z80MemAddr addr, const vector<u8>& buffer);
    void                load                (Z80MemAddr addr, const void* buffer, i64 size);
    void                setRomWriteState    (bool writable);
    vector<MemAddr>     findSequence        (vector<u8> seq);
    vector<MemAddr>     findByte            (u8 byte);
    vector<MemAddr>     findWord            (u16 word);
    vector<MemAddr>     findString          (string str);
    MemAddr             convertAddress      (Z80MemAddr addr) const;
    Z80MemAddr          convertAddress      (MemAddr addr) const;
    bool                isZ80Address        (MemAddr addr) const;
    string              addressName         (MemAddr address);
    u8&                 memRef              (MemAddr addr);
    vector<u8>          getMmu              (MemGroup group, int index);
    void                setMmu              (MemGroup group, int index, const vector<u8>& data);

    //------------------------------------------------------------------------------------------------------------------
    // Video interface
    //------------------------------------------------------------------------------------------------------------------

    void            recalcVideoMaps     ();

    //------------------------------------------------------------------------------------------------------------------
    // I/O interface
    //------------------------------------------------------------------------------------------------------------------

    void            ioContend           (u16 port, TState delay, int num, TState& t);

    //------------------------------------------------------------------------------------------------------------------
    // Kempston interface
    //------------------------------------------------------------------------------------------------------------------

    void            setKempstonState(u8 state);
    u8              getKempstonState() const;

    //------------------------------------------------------------------------------------------------------------------
    // Debugger interface
    //------------------------------------------------------------------------------------------------------------------

    void            toggleBreakpoint        (MemAddr address);
    void            addTemporaryBreakpoint  (MemAddr address);
    bool            hasUserBreakpointAt     (MemAddr address) const;
    vector<MemAddr> getUserBreakpoints      () const;
    void            clearUserBreakpoints    ();

    struct DataBreakpoint
    {
        MemAddr address;
        u16     len;
    };

    void            toggleDataBreakpoint    (MemAddr address, u16 len);
    bool            hasDataBreakpoint       (MemAddr address, u16 len) const;
    const vector<DataBreakpoint>&
                    getDataBreakpoints      () const { return m_dataBreakpoints; }
    void            clearDataBreakpoints    () { m_dataBreakpoints.clear(); }

private:
    //
    // Memory
    //
    void            initMemory          ();
    vector<u8>&     getMemoryGroup      (MemGroup group);

    //
    // Video
    //
    void            initVideo           ();
    void            updateVideo         ();

    //
    // Audio
    //
    void            initAudio           ();

    //
    // I/O
    //
    void            initIo              ();

    //
    // Tape
    //
    void            updateTape          (TState numTStates);

    //
    // Breakpoints
    //
    enum class BreakpointType
    {
        User,       // User added breakpoint, only user can remove it
        Temporary,  // System added breakpoint, and it should be removed when hit.
    };

    struct Breakpoint
    {
        BreakpointType  type;
        MemAddr         address;
    };

    vector<Breakpoint>::const_iterator      findBreakpoint          (MemAddr address) const;
    bool                                    shouldBreak             (MemAddr address);

    vector<DataBreakpoint>::const_iterator  findDataBreakpoint      (MemAddr address, u16 len) const;

private:
    // Model
    Model                       m_model;

    // Clock state
    TState                      m_tState;

    // Video state
    int                         m_videoBank;
    int                         m_shadowVideoBank;
    u32*                        m_image;
    sf::Texture                 m_videoTexture;
    sf::Sprite                  m_videoSprite;
    u8                          m_frameCounter;
    vector<u16>                 m_videoMap;         // Maps t-states to addresses
    vector<u16>                 m_shadowVideoMap;   // Maps t-states to addresses
    int                         m_videoWrite;       // Write point into 2D image array
    TState                      m_startTState;      // Starting t-state for top-left of window
    TState                      m_drawTState;       // Current t-state that has been draw to

    // Audio state
    Audio                       m_audio;
    Tape*                       m_tape;

    // Memory state
    vector<Bank>                m_slots;
    vector<string>              m_bankNames;
    vector<u8>                  m_rom;
    vector<u8>                  m_ram;
    vector<u8>                  m_contention;
    bool                        m_romWritable;

    // CPU state
    Z80                         m_z80;

    // ULA state
    u8                          m_borderColour;
    vector<u8>                  m_keys;
    u8                          m_speaker;
    u8                          m_tapeEar;

    // 128K paging state
    bool                        m_pagingDisabled;
    bool                        m_shadowScreen;

    // Debugger state
    vector<Breakpoint>          m_breakpoints;
    vector<DataBreakpoint>      m_dataBreakpoints;
    bool                        m_break;

    // Kempston
    bool                        m_kempstonJoystick;
    u8                          m_kempstonState;
};
