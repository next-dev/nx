//----------------------------------------------------------------------------------------------------------------------
// Base class for all models of Spectrum
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"
#include "config.h"
#include "z80.h"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

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
// Spectrum base class
// Each model must override this and implement the specifics
//----------------------------------------------------------------------------------------------------------------------

class Spectrum: public IExternals
{
public:
    // TState counter
    using TState        = i64;

    //------------------------------------------------------------------------------------------------------------------
    // Construction/Destruction
    //------------------------------------------------------------------------------------------------------------------

    Spectrum();
    virtual ~Spectrum();

    //------------------------------------------------------------------------------------------------------------------
    // Control
    //------------------------------------------------------------------------------------------------------------------
    
    void            reset               (bool hard = true);

    //------------------------------------------------------------------------------------------------------------------
    // State
    //------------------------------------------------------------------------------------------------------------------

    sf::Sprite&     getVideoSprite      ();
    TState          getFrameTime        () const { return 69888; }
    u8              getBorderColour     () const { return m_borderColour; }


    //------------------------------------------------------------------------------------------------------------------
    // IExternals interface
    //------------------------------------------------------------------------------------------------------------------

    u8              peek                (u16 address) override;
    u8              peek                (u16 address, TState& t) override;
    u16             peek16              (u16 address, TState& t) override;
    void            poke                (u16 address, u8 x, TState& t) override;
    void            poke16              (u16 address, u16 x, TState& t) override;
    void            contend             (u16 address, TState delay, int num, TState& t) override;
    u8              in                  (u16 port, TState& t) override;
    void            out                 (u16 port, u8 x, TState& t) override;

    //------------------------------------------------------------------------------------------------------------------
    // General functionality, not specific to a model
    //------------------------------------------------------------------------------------------------------------------

    // Main function called to generate a single frame or instruction (in single-step mode).
    void            update              ();

    // Emulation control
    void            togglePause         ();
    
    // Set the keyboard state.
    void            setKeyboardState    (vector<u8>& rows);

    //------------------------------------------------------------------------------------------------------------------
    // Memory interface
    //------------------------------------------------------------------------------------------------------------------

    bool            isContended         (u16 addr) const;
    TState          contention          (TState t);
    void            poke                (u16 address, u8 x);
    void            load                (u16 address, const void* buffer, i64 size);
    void            setRomWriteState    (bool writable);

    //------------------------------------------------------------------------------------------------------------------
    // I/O interface
    //------------------------------------------------------------------------------------------------------------------

    void            ioContend           (u16 port, TState delay, int num, TState& t);

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

private:

    // Clock state
    TState          m_tState;
    TState          m_lastTState;

    // Video state
    u32*            m_image;
    sf::Texture     m_videoTexture;
    sf::Sprite      m_videoSprite;
    u8              m_frameCounter;
    vector<u16>     m_videoMap;         // Maps t-states to addresses
    int             m_videoWrite;       // Write point into 2D image array
    TState          m_startTState;      // Starting t-state for top-left of window
    TState          m_drawTState;       // Current t-state that has been draw to

    // Memory state
    vector<u8>      m_ram;
    vector<u8>      m_contention;
    bool            m_romWritable;

    // CPU state
    Z80             m_z80;

    // ULA state
    u8              m_borderColour;
    vector<u8>      m_keys;
};
