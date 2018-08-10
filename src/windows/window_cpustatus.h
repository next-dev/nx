//----------------------------------------------------------------------------------------------------------------------
//! @file       windows/window_cpustatus.h
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Defines the CPU status window that appears in the debugger.

#pragma once

#include <ui/window.h>

//----------------------------------------------------------------------------------------------------------------------
//! The CPU status window inside the debugger.
//!
//! This window shows all the CPU registers, flags, stack and memory paging configuration.  Clicking on a register or
//! flag allow you to change it.  Registers can be changed via commands.

class CpuStatusWindow : public Window
{
public:
    CpuStatusWindow(Nx& nx);

protected:
    void onRender(Draw& draw) override;
    bool onKey(const KeyEvent& kev) override;
    void onText(char ch) override;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
