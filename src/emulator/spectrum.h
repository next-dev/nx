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
    Bank(MemGroup group, u16 bank);

    MemGroup getGroup() const { return m_group; }
    u16 getBankIndex() const { return m_bank; }

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
    MemAddr(Bank bank, u16 offset);

    Bank bank() const       { return m_bank; }
    u16 offset() const      { return m_offset;}

    i64 realAddress() const { return m_bank.getBankIndex() * kBankSize + m_offset; }

private:
    Bank        m_bank;
    u16         m_offset;
};

//
// Z80MemAddr
//
// This is an address that address the 64K that the Z80 can see.  This address needs to be converted via the
// Spectrum instance to a MemAddr before it can be used (because the Spectrum instance has the slot configuration).
//

class Z80MemAddr
{
public:
    Z80MemAddr(u16 addr);

    operator u16() const { return m_address; }

private:
    u16     m_address;
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

    int                 getBankSize         () const    { return m_bankSize; }
    int                 getNumSlots         () const    { return (int)m_slots.size(); }
    int                 getRomSize          () const    { return 0x4000; }
    void                bank                (int slot, int bank);
    int                 getBank             (int slot) const;
    u16                 getNumBanks         () const;
    string&             slotName            (int slot);
    bool                isContended         (u16 addr) const;
    TState              contention          (TState t);
    void                poke                (u16 address, u8 x);
    u8                  fullPeek            (MemAddr addr) const;
    void                fullPoke            (MemAddr addr, u8 byte);
    void                load                (Z80MemAddr addr, const vector<u8>& buffer);
    void                load                (Z80MemAddr addr, const void* buffer, i64 size);
    void                setRomWriteState    (bool writable);
    vector<Z80MemAddr>  findSequence        (vector<u8> seq);
    vector<Z80MemAddr>  findByte            (u8 byte);
    vector<Z80MemAddr>  findWord            (u16 word);
    vector<Z80MemAddr>  findString          (string str);
    MemAddr             convertAddress      (Z80MemAddr addr) const;
    Z80MemAddr          convertAddress      (MemAddr addr) const;
    string              addressName         (MemAddr address, bool moreInfo);

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
    vector<u16>     getUserBreakpoints      () const;
    void            clearUserBreakpoints    ();

    struct DataBreakpoint
    {
        u16     address;
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
        u16             address;
    };

    vector<Breakpoint>::const_iterator      findBreakpoint          (MemAddr address) const;
    bool                                    shouldBreak             (MemAddr address);

    vector<DataBreakpoint>::const_iterator  findDataBreakpoint  (MemAddr address, u16 len) const;

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
    vector<u16>                 m_slots;
    vector<string>              m_bankNames;
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
