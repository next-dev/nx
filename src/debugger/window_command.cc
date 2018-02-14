//----------------------------------------------------------------------------------------------------------------------
// Command window
//----------------------------------------------------------------------------------------------------------------------

#include <debugger/overlay_debugger.h>
#include <emulator/nx.h>

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

CommandWindow::CommandWindow(Nx& nx)
    : SelectableWindow(nx, 45, 22, 34, 30, "Command Window", Colour::Blue, Colour::Black)
    , m_commandEditor(46, 23, 32, 28, Draw::attr(Colour::White, Colour::Black, false), false, 1024, 1024)
{
    m_commandEditor.getData().insert('>');
    m_commandEditor.getData().insert(' ');
}

//----------------------------------------------------------------------------------------------------------------------
// Window overrides
//----------------------------------------------------------------------------------------------------------------------

void CommandWindow::onDraw(Draw& draw)
{
    m_commandEditor.renderAll(draw);
}

void CommandWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    m_commandEditor.key(key, down, shift, ctrl, alt);
}

void CommandWindow::onText(char ch)
{
    m_commandEditor.text(ch);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
