//----------------------------------------------------------------------------------------------------------------------
//! @file       windows/window_command.h
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Defines the command window that appears in the debugger.

#pragma once

#include <ui/window.h>

//----------------------------------------------------------------------------------------------------------------------
//! The command window inside the debugger.
//!
//! The command window has a full Nerd Scripting environment built in with many commands able to drive the emulator.

class CommandWindow : public Window
{
public:
    CommandWindow(Nx& nx);

protected:
    void onRender(Draw& draw) override;
    bool onKey(const KeyEvent& kev) override;
    void onText(char ch) override;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
