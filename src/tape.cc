//----------------------------------------------------------------------------------------------------------------------
// Tape browser
//----------------------------------------------------------------------------------------------------------------------

#include "tape.h"
#include "nx.h"

//----------------------------------------------------------------------------------------------------------------------
// Tape
//----------------------------------------------------------------------------------------------------------------------

Tape::Tape()
{

}

Tape::Tape(const vector<u8>& data)
{

}

//----------------------------------------------------------------------------------------------------------------------
// Tape window
//----------------------------------------------------------------------------------------------------------------------

TapeWindow::TapeWindow(Nx& nx)
    : Window(nx, 1, 1, 40, 60, "Tape Browser", Colour::Black, Colour::White, true)
    , m_topIndex(0)
    , m_index(0)
{

}

void TapeWindow::reset()
{
    m_index = m_topIndex = 0;
}

void TapeWindow::onDraw(Draw& draw)
{

}

void TapeWindow::onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt)
{

}

void TapeWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
// Tape browser overlay
//----------------------------------------------------------------------------------------------------------------------

TapeBrowser::TapeBrowser(Nx& nx)
    : Overlay(nx)
    , m_window(nx)
    , m_commands({
        "Esc|Exit",
        })
    , m_currentTape(nullptr)
{

}

void TapeBrowser::loadTape(const vector<u8>& data)
{
    if (m_currentTape)
    {
        delete m_currentTape;
    }

    m_currentTape = new Tape(data);
}

void TapeBrowser::render(Draw& draw)
{
    m_window.draw(draw);
}

void TapeBrowser::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    if (down && !shift && !ctrl && !alt)
    {
        using K = sf::Keyboard::Key;
        switch (key)
        {
        case K::Escape:
            getEmulator().hideAll();
            break;
        }
    }
}

void TapeBrowser::text(char ch)
{

}

const vector<string>& TapeBrowser::commands() const
{
    return m_commands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
