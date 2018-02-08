//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/overlay_asm.h>

//----------------------------------------------------------------------------------------------------------------------
// AssemblerWindow
//----------------------------------------------------------------------------------------------------------------------

AssemblerWindow::AssemblerWindow(Nx& nx)
    : Window(nx, 1, 1, 78, 60, "Assembler Results", Colour::Blue, Colour::Black, false)
{

}

void AssemblerWindow::onDraw(Draw& draw)
{

}

void AssemblerWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{

}

void AssemblerWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
// AssemblerOverlay
//----------------------------------------------------------------------------------------------------------------------

AssemblerOverlay::AssemblerOverlay(Nx& nx)
    : Overlay(nx)
    , m_window(nx)
    , m_commands({
            "Any Key|Exit results",
        })
{
}

void AssemblerOverlay::render(Draw& draw)
{
    m_window.draw(draw);
}

void AssemblerOverlay::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    m_window.keyPress(key, down, shift, ctrl, alt);
}

void AssemblerOverlay::text(char ch)
{
    m_window.text(ch);
}

const vector<string>& AssemblerOverlay::commands() const
{
    return m_commands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
