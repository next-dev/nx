//----------------------------------------------------------------------------------------------------------------------
//! @file       overlays/overlays_debugger.cc
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Implements the debugger overlay.

#include <overlays/overlay_debugger.h>

//----------------------------------------------------------------------------------------------------------------------

DebuggerOverlay::DebuggerOverlay(Nx& nx)
    : Overlay(nx)
    , m_disassemblyWindow(nx)
    , m_memoryDumpWindow(nx)
    , m_cpuStatusWindow(nx)
    , m_commandWindow(nx)
    , m_currentWindow(&m_memoryDumpWindow)
{
}

//----------------------------------------------------------------------------------------------------------------------

void DebuggerOverlay::apply(const FrameState& frameState)
{
    Overlay::apply(frameState);
    recalculateWindows();
}

//----------------------------------------------------------------------------------------------------------------------

void DebuggerOverlay::onRender(Draw& draw)
{
    m_disassemblyWindow.render(draw);
    m_memoryDumpWindow.render(draw);
    m_cpuStatusWindow.render(draw);
    m_commandWindow.render(draw);
}

//----------------------------------------------------------------------------------------------------------------------

bool DebuggerOverlay::onKey(const KeyEvent& kev)
{
    using K = sf::Keyboard::Key;

    if (kev.isNormal() && kev.key == K::Tilde)
    {
        exit();
        return true;
    }
    else
    {
        return m_currentWindow->key(kev);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void DebuggerOverlay::onText(char ch)
{
    m_currentWindow->text(ch);
}

//----------------------------------------------------------------------------------------------------------------------

void DebuggerOverlay::recalculateWindows()
{
    Window::State state;

    // Memory dump window
    state.x = 1;
    state.y = 1;
    state.width = 43;
    state.height = 20;
    state.title = "Memory Viewer (Alt-1)";
    state.ink = Colour::Black;
    state.paper = Colour::White;
    state.selected = true;
    m_memoryDumpWindow.apply(state);

    // Disassembly window
    state.x = 1;
    state.y = 22;
    state.width = 43;
    state.height = getCellHeight() - state.y - 4;
    state.title = "Disassembly View (Alt-2)";
    state.selected = false;
    m_disassemblyWindow.apply(state);

    // CPU status window
    state.x = getCellWidth() - 1 - 34;
    state.y = 1;
    state.width = getCellWidth() - 1 - state.x;
    state.height = 20;
    state.title = "CPU status (Alt-3)";
    m_cpuStatusWindow.apply(state);

    // Command window
    state.x = 45;
    state.y = 22;
    state.width = getCellWidth() - state.x - 1;
    state.height = getCellHeight() - state.y - 4;
    state.title = "Command window (Alt-4)";
    m_commandWindow.apply(state);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

