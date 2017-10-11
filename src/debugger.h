//----------------------------------------------------------------------------------------------------------------------
// Debugger windows
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

#include "ui.h"

//----------------------------------------------------------------------------------------------------------------------
// Memory dump
//----------------------------------------------------------------------------------------------------------------------

class MemoryDumpWindow final : public SelectableWindow
{
public:
    MemoryDumpWindow(Spectrum& speccy);

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key) override;

private:
    u16     m_address;
};

//----------------------------------------------------------------------------------------------------------------------
// Disassembler
//----------------------------------------------------------------------------------------------------------------------

class Disassembler;

class DisassemblyWindow final : public SelectableWindow
{
public:
    DisassemblyWindow(Spectrum& speccy);

    void adjustBar();
    void setCursor(u16 address);

private:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key) override;

    u16 backInstruction(u16 address);
    u16 disassemble(Disassembler& d, u16 address);

    void setView(u16 newTopAddress);
    int findViewAddress(u16 address);
    void cursorDown();
    void cursorUp();

private:
    u16     m_topAddress;
    u16     m_address;

    // This is used to improve cursor movement on disassembly.
    std::vector<u16> m_viewedAddresses;
};

//----------------------------------------------------------------------------------------------------------------------
// CPU Status
//----------------------------------------------------------------------------------------------------------------------

class Z80;

class CpuStatusWindow final : public Window
{
public:
    CpuStatusWindow(Spectrum& speccy);

private:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key) override;

protected:
    Z80&        m_z80;
};

//----------------------------------------------------------------------------------------------------------------------
// Debugger
//----------------------------------------------------------------------------------------------------------------------

class Debugger
{
public:
    Debugger(Spectrum& speccy);

    void onKey(sf::Keyboard::Key key);
    void render();
    sf::Sprite& getSprite() { return m_ui.getSprite(); }

    MemoryDumpWindow&   getMemoryDumpWindow() { return m_memoryDumpWindow; }
    DisassemblyWindow&  getDisassemblyWindow() { return m_disassemblyWindow; }
    CpuStatusWindow&    getCpuStatusWindow() { return m_cpuStatusWindow; }

private:
    Ui                  m_ui;
    MemoryDumpWindow    m_memoryDumpWindow;
    DisassemblyWindow   m_disassemblyWindow;
    CpuStatusWindow     m_cpuStatusWindow;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
