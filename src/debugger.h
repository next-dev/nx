//----------------------------------------------------------------------------------------------------------------------
// Debugger windows
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

#include "ui.h"

enum class DebugKey
{
    Left,
    Down,
    Up,
    Right,
    PageUp,
    PageDn,
    Tab,

    COUNT
};

//----------------------------------------------------------------------------------------------------------------------
// Window base class
//----------------------------------------------------------------------------------------------------------------------

class Machine;

class Window
{
public:
    Window(Machine& M, int x, int y, int width, int height, std::string title, Colour ink, Colour paper, bool bright);

    virtual void draw(Ui::Draw& draw);
    virtual void keyPress(DebugKey key, bool down);

protected:
    //
    // Hooks
    //
    virtual void onDraw(Ui::Draw& draw) = 0;
    virtual void onKey(DebugKey key, bool down) = 0;

protected:
    Machine&        m_machine;
    int             m_x;
    int             m_y;
    int             m_width;
    int             m_height;
    std::string     m_title;
    u8              m_bkgColour;
};

//----------------------------------------------------------------------------------------------------------------------
// Selectable window
//----------------------------------------------------------------------------------------------------------------------

class SelectableWindow : public Window
{
public:
    SelectableWindow(Machine& M, int x, int y, int width, int height, std::string title, Colour ink, Colour paper);

    void Select();
    bool isSelected() const { return ms_currentWindow == this; }

    void draw(Ui::Draw& draw) override;
    void keyPress(DebugKey key, bool down);

private:
    static SelectableWindow* ms_currentWindow;
};

//----------------------------------------------------------------------------------------------------------------------
// Memory dump
//----------------------------------------------------------------------------------------------------------------------

class MemoryDumpWindow final: public SelectableWindow
{
public:
    MemoryDumpWindow(Machine& M);

protected:
    void onDraw(Ui::Draw& draw) override;
    void onKey(DebugKey key, bool down) override;

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
    DisassemblyWindow(Machine& M);

private:
    void onDraw(Ui::Draw& draw) override;
    void onKey(DebugKey key, bool down) override;

    u16 backInstruction(u16 address);
    u16 disassemble(Disassembler& d, u16 address);

private:
    u16     m_address;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <sstream>
#include <iomanip>

#include "nx.h"

//----------------------------------------------------------------------------------------------------------------------
// Window base class
//----------------------------------------------------------------------------------------------------------------------

Window::Window(Machine& M, int x, int y, int width, int height, std::string title, Colour ink, Colour paper, bool bright)
    : m_machine(M)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_title(title)
    , m_bkgColour((int)ink + (8 * (int)paper) + (bright ? 0x40 : 0x00))
{

}

void Window::draw(Ui::Draw& draw)
{
    draw.window(m_x, m_y, m_width, m_height, m_title.c_str(), (m_bkgColour & 0x40) != 0, m_bkgColour);
    onDraw(draw);
}

void Window::keyPress(DebugKey key, bool down)
{
    onKey(key, down);
}

//----------------------------------------------------------------------------------------------------------------------
// Selectable window class
//----------------------------------------------------------------------------------------------------------------------

SelectableWindow* SelectableWindow::ms_currentWindow = 0;

SelectableWindow::SelectableWindow(Machine& M, int x, int y, int width, int height, std::string title, Colour ink, Colour paper)
    : Window(M, x, y, width, height, title, ink, paper, false)
{

}

void SelectableWindow::draw(Ui::Draw& draw)
{
    u8 bkg = m_bkgColour;
    if (ms_currentWindow == this)
    {
        bkg |= 0x40;
    }
    else
    {
        bkg &= ~0x40;
    }
    draw.window(m_x, m_y, m_width, m_height, m_title.c_str(), (bkg & 0x40) != 0, bkg);
    onDraw(draw);
}

void SelectableWindow::keyPress(DebugKey key, bool down)
{
    if (ms_currentWindow == this)
    {
        onKey(key, down);
    }
}

void SelectableWindow::Select()
{
    if (ms_currentWindow)
    {
        ms_currentWindow->m_bkgColour &= ~0x40;
    }
    ms_currentWindow = this;
    m_bkgColour |= 0x40;
}

//----------------------------------------------------------------------------------------------------------------------
// Memory dump class
//----------------------------------------------------------------------------------------------------------------------

MemoryDumpWindow::MemoryDumpWindow(Machine& M)
    : SelectableWindow(M, 1, 1, 43, 20, "Memory Viewer", Colour::Black, Colour::White)
    , m_address(0)
{

}

void MemoryDumpWindow::onDraw(Ui::Draw& draw)
{
    u16 a = m_address;
    for (int i = 1; i < m_height - 1; ++i, a += 8)
    {
        using namespace std;
        stringstream ss;
        ss << setfill('0') << setw(4) << hex << uppercase << a << " : ";
        for (int b = 0; b < 8; ++b)
        {
            ss << setfill('0') << setw(2) << hex << uppercase << (int)m_machine.getMemory().peek(a + b) << " ";
        }
        ss << "  ";
        for (int b = 0; b < 8; ++b)
        {
            char ch = m_machine.getMemory().peek(a + b);
            ss << ((ch < 32 || ch > 127) ? '.' : ch);
        }
        draw.printString(m_x + 1, m_y + i, ss.str().c_str(), m_bkgColour);
    }
}

void MemoryDumpWindow::onKey(DebugKey key, bool down)
{
    if (down)
    {
        using DK = DebugKey;
        switch (key)
        {
        case DK::Up:
            m_address -= 8;
            break;

        case DK::Down:
            m_address += 8;
            break;

        case DK::PageUp:
            m_address -= (m_height - 2) * 8;
            break;

        case DK::PageDn:
            m_address += (m_height - 2) * 8;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Disassembler class
//----------------------------------------------------------------------------------------------------------------------

#include "disasm.h"

DisassemblyWindow::DisassemblyWindow(Machine& M)
    : SelectableWindow(M, 1, 22, 43, 30, "Disassembly", Colour::Black, Colour::White)
    , m_address(0)
{

}

void DisassemblyWindow::onDraw(Ui::Draw& draw)
{
    Disassembler d;
    u16 a = m_address;

    u8 bkg2 = m_bkgColour & ~0x40;
    for (int row = 1; row < m_height - 1; ++row)
    {
        u16 next = disassemble(d, a);
        u8 colour = row & 1 ? m_bkgColour : bkg2;

        draw.attrRect(m_x, m_y + row, m_width, 1, colour);
        draw.printString(m_x + 1, m_y + row, d.addressAndBytes(a).c_str(), colour);
        draw.printString(m_x + 20, m_y + row, d.opCode().c_str(), colour);
        draw.printString(m_x + 25, m_y + row, d.operands().c_str(), colour);
        a = next;
    }

}

void DisassemblyWindow::onKey(DebugKey key, bool down)
{
    if (down)
    {
        using DK = DebugKey;
        switch (key)
        {
        case DK::Up:
            m_address = backInstruction(m_address);
            break;

        case DK::Down:
            {
                Disassembler d;
                m_address = disassemble(d, m_address);
            }
            break;

        case DK::PageUp:
            for (int i = 0; i < (m_height - 2); ++i)
            {
                m_address = backInstruction(m_address);
            }
            break;

        case DK::PageDn:
            for (int i = 0; i < (m_height - 2); ++i)
            {
                Disassembler d;
                m_address = disassemble(d, m_address);
            }
        }
    }
}

u16 DisassemblyWindow::backInstruction(u16 address)
{
    u16 count = 1;
    Disassembler d;

    // Keep disassembling back until the address after the instruction is equal
    // to the current one
    while (count < 4) {
        u16 a = address - count;
        a = disassemble(d, a);
        if (a == address)
        {
            // This instruction will do!
            return address - count;
        }
        ++count;
    }

    // Couldn't find a suitable instruction, fallback: just go back one byte
    return address - 1;
}

u16 DisassemblyWindow::disassemble(Disassembler& d, u16 address)
{
    return d.disassemble(address,
        m_machine.getMemory().peek(address + 0),
        m_machine.getMemory().peek(address + 1),
        m_machine.getMemory().peek(address + 2),
        m_machine.getMemory().peek(address + 3));
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
