//----------------------------------------------------------------------------------------------------------------------
// Emulation of a tape and tape deck
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "config.h"
#include "types.h"

#include "ui.h"

#include <string>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
// A tape
// Contains blocks and converts t-state advances into EAR signals
//----------------------------------------------------------------------------------------------------------------------

class Tape
{
public:
    Tape();
    Tape(const vector<u8>& data);
};

//----------------------------------------------------------------------------------------------------------------------
// TapeWindow
//----------------------------------------------------------------------------------------------------------------------

class TapeWindow final : public Window
{
public:
    TapeWindow(Nx& nx);

    void reset();

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

private:
    int m_topIndex;
    int m_index;
};

//----------------------------------------------------------------------------------------------------------------------
// A tape-browser overlay
// Contains a single tape, and allows controls
//----------------------------------------------------------------------------------------------------------------------

class TapeBrowser final : public Overlay
{
public:
    TapeBrowser(Nx& nx);

    void loadTape(const vector<u8>& data);

protected:
    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

private:
    TapeWindow      m_window;
    vector<string>  m_commands;
    Tape*           m_currentTape;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

