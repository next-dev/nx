//----------------------------------------------------------------------------------------------------------------------
// Debugger
//----------------------------------------------------------------------------------------------------------------------

#include "debugger.h"
#include "nx.h"

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Debugger::Debugger(Nx& nx)
    : Overlay(nx)
    , m_memoryDumpWindow(nx.getSpeccy())
    , m_disassemblyWindow(nx.getSpeccy())
    , m_cpuStatusWindow(nx.getSpeccy())
    , m_memoryDumpCommands({
        "Up|Scroll up",
        "Down|Scroll down",
        "PgUp|Page up",
        "PgDn|Page down",
        "~|Exit",
        "Tab|Switch window"})
    , m_disassemblyCommands({
        "G|oto",
        "F5|Pause/Run",
        "F6|Step Over",
        "F7|Step In",
        "F9|Breakpoint",
        "Up|Scroll up",
        "Down|Scroll down",
        "PgUp|Page up",
        "PgDn|Page down",
        "~|Exit",
        "Tab|Switch window"})
{
    m_disassemblyWindow.Select();
}

//----------------------------------------------------------------------------------------------------------------------
// Keyboard handling
//----------------------------------------------------------------------------------------------------------------------

void Debugger::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    if (!down) return;

    switch (key)
    {
    case K::Tilde:
        getEmulator().toggleDebugger();
        break;

    case K::F5:
        getEmulator().togglePause(false);
        break;

    case K::F6:
        getEmulator().stepOver();
        break;

    case K::F7:
        getEmulator().stepIn();
        break;

    case K::Tab:
        // #todo: put cycling logic into SelectableWindow
        if (getDisassemblyWindow().isSelected())
        {
            getMemoryDumpWindow().Select();
        }
        else
        {
            getDisassemblyWindow().Select();
        }
        break;

    default:
        SelectableWindow::getSelected().keyPress(key, shift, ctrl, alt);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------------------------------------------------

void Debugger::render(Draw& draw)
{
    m_memoryDumpWindow.draw(draw);
    m_disassemblyWindow.draw(draw);
    m_cpuStatusWindow.draw(draw);
}

//----------------------------------------------------------------------------------------------------------------------
// Commands
//----------------------------------------------------------------------------------------------------------------------

const vector<string>& Debugger::commands() const
{
    return m_memoryDumpWindow.isSelected() ? m_memoryDumpCommands : m_disassemblyCommands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
