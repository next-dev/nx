//----------------------------------------------------------------------------------------------------------------------
//! @file       windows/window_disassembly.h
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Defines the disassembly window that appears in the debugger.

#pragma once

#include <ui/window.h>

//----------------------------------------------------------------------------------------------------------------------
//! The disassembly window inside the debugger.

class DisassemblyWindow : public Window
{
public:
    DisassemblyWindow(Nx& nx);

protected:
    void onRender(Draw& draw) override;
    bool onKey(const KeyEvent& kev) override;
    void onText(char ch) override;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
