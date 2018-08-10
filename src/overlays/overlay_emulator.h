//----------------------------------------------------------------------------------------------------------------------
//! Defines the EmulatorOverlay overlay.
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <ui/overlay.h>

//----------------------------------------------------------------------------------------------------------------------
//! All the keys on the normal Spectrum keyboard.

enum class SpeccyKey
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
//! The emulator's overlay.
//!
//! This is the default overlay for when there isn't an overlay shown (e.g. debugger, editor etc).  It's job is to
//! capture all keyboard & mouse input and send it to the emulator.
//----------------------------------------------------------------------------------------------------------------------

class EmulatorOverlay : public Overlay
{
public:
    EmulatorOverlay(Nx& nx);

    void apply(const FrameState& frameState) override;

    //! Clear the key status.
    void clearKeys();

protected:
    void onRender(Draw& draw) override;
    bool onKey(const KeyEvent& kev) override;
    void onText(char ch) override;

private:
    //! Calculate the key rows from the key states.
    void calculateKeys();

    //! Open any file.
    void openFile(string fileName);

private:
    // Keyboard state
    bool    m_speccyKeys[(int)SpeccyKey::COUNT];        //!< State of spectrum keys.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
