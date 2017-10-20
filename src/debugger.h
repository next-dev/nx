//----------------------------------------------------------------------------------------------------------------------
// Debugger windows
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

#include "editor.h"

//----------------------------------------------------------------------------------------------------------------------
// Memory dump
//----------------------------------------------------------------------------------------------------------------------

class MemoryDumpWindow final : public SelectableWindow
{
public:
    MemoryDumpWindow(Nx& nx);

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;
    void onUnselected() override;

private:
    u16     m_address;
    Editor  m_gotoEditor;
    int     m_enableGoto;
};

//----------------------------------------------------------------------------------------------------------------------
// Disassembler
//----------------------------------------------------------------------------------------------------------------------

class Disassembler;

class DisassemblyWindow final : public SelectableWindow
{
public:
    DisassemblyWindow(Nx& nx);

    void adjustBar();
    void setCursor(u16 address);
    u16 disassemble(Disassembler& d, u16 address);
    u16 disassemble(u16 address);

private:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;
    void onUnselected() override;

    u16 backInstruction(u16 address);

    void setView(u16 newTopAddress);
    int findViewAddress(u16 address);
    void cursorDown();
    void cursorUp();

private:
    u16     m_topAddress;
    u16     m_address;

    // This is used to improve cursor movement on disassembly.
    std::vector<u16> m_viewedAddresses;
    
    Editor  m_gotoEditor;
    int     m_enableGoto;
};

//----------------------------------------------------------------------------------------------------------------------
// CPU Status
//----------------------------------------------------------------------------------------------------------------------

class Z80;

class CpuStatusWindow final : public Window
{
public:
    CpuStatusWindow(Nx& nx);

private:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

protected:
    Z80&        m_z80;
};

//----------------------------------------------------------------------------------------------------------------------
// Debugger
//----------------------------------------------------------------------------------------------------------------------

class Nx;

class Debugger : public Overlay
{
public:
    Debugger(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

    MemoryDumpWindow&   getMemoryDumpWindow() { return m_memoryDumpWindow; }
    DisassemblyWindow&  getDisassemblyWindow() { return m_disassemblyWindow; }
    CpuStatusWindow&    getCpuStatusWindow() { return m_cpuStatusWindow; }

private:
    MemoryDumpWindow    m_memoryDumpWindow;
    DisassemblyWindow   m_disassemblyWindow;
    CpuStatusWindow     m_cpuStatusWindow;

    vector<string>      m_memoryDumpCommands;
    vector<string>      m_disassemblyCommands;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
