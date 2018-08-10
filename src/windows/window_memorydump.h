//----------------------------------------------------------------------------------------------------------------------
//! @file       windows/window_memorydump.h
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Defines the memory dump window that appears in the debugger.

#pragma once

#include <ui/window.h>

//----------------------------------------------------------------------------------------------------------------------
//! The memory dump window inside the debugger.
//!
//! The memory dump window allows you to view all memory, and switch between Z80 mode and paged mode (8K and 16K slots).
//! You can also edit the data in the memory dump window, and it will show all current searches.

class MemoryDumpWindow : public Window
{
public:
    MemoryDumpWindow(Nx& nx);

protected:
    void onRender(Draw& draw) override;
    bool onKey(const KeyEvent& kev) override;
    void onText(char ch) override;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
