//----------------------------------------------------------------------------------------------------------------------
//! @file       overlays/overlay_debugger.h
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Defines the debugger overlay.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <ui/overlay.h>
#include <windows/window_disassembly.h>
#include <windows/window_memorydump.h>
#include <windows/window_cpustatus.h>
#include <windows/window_command.h>

//----------------------------------------------------------------------------------------------------------------------
//! Debugger overlay.
//!
//! The debugger contains 4 main views: the disassembler, the memory dump, the CPU status and the command window.
//! Each can be zoomed by pressing Ctrl+Z.

class DebuggerOverlay : public Overlay
{
public:
    DebuggerOverlay(Nx& nx);

    void apply(const FrameState& frameState) override;

    DisassemblyWindow& getDisassemblyWindow() { return m_disassemblyWindow; }
    MemoryDumpWindow& getMemoryDumpWindow() { return m_memoryDumpWindow; }
    CpuStatusWindow& getCpuStatusWindow() { return m_cpuStatusWindow; }
    CommandWindow& getCommandWindow() { return m_commandWindow; }

    // Const versions because.... sigh..... C++.
    const DisassemblyWindow& getDisassemblyWindow() const { return m_disassemblyWindow; }
    const MemoryDumpWindow& getMemoryDumpWindow() const { return m_memoryDumpWindow; }
    const CpuStatusWindow& getCpuStatusWindow() const { return m_cpuStatusWindow; }
    const CommandWindow& getCommandWindow() const { return m_commandWindow; }

protected:
    void onRender(Draw& draw) override;
    bool onKey(const KeyEvent& kev) override;
    void onText(char ch) override;

private:
    DisassemblyWindow   m_disassemblyWindow;
    MemoryDumpWindow    m_memoryDumpWindow;
    CpuStatusWindow     m_cpuStatusWindow;
    CommandWindow       m_commandWindow;
};
