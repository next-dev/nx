//----------------------------------------------------------------------------------------------------------------------
// Debugger windows
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

#include "ui.h"

//----------------------------------------------------------------------------------------------------------------------
// Memory dump
//----------------------------------------------------------------------------------------------------------------------

class MemoryDumpWindow final: public SelectableWindow
{
public:
    MemoryDumpWindow(Machine& M);

protected:
    void onDraw(Ui::Draw& draw) override;
    void onKey(UiKey key, bool down) override;

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
    
    void adjustBar();
    void setCursor(u16 address);

private:
    void onDraw(Ui::Draw& draw) override;
    void onKey(UiKey key, bool down) override;

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

class CpuStatusWindow final: public Window
{
public:
    CpuStatusWindow(Machine& M);
    
private:
    void onDraw(Ui::Draw& draw) override;
    void onKey(UiKey key, bool down) override;
    
protected:
    Z80&        m_z80;
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

void MemoryDumpWindow::onKey(UiKey key, bool down)
{
    if (down)
    {
        using DK = UiKey;
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
                
        default:
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
    , m_topAddress(0)
{

}

void DisassemblyWindow::adjustBar()
{
    if (m_address < m_topAddress)
    {
        setView(m_address);
    }
    
    // Calculate which row our bar is on.
    int row = 0;
    u16 a = m_topAddress;
    Disassembler d;
    
    int index = findViewAddress(a);
    
    for (row = 1; row < m_height-1; ++row)
    {
        if (index == -1)
        {
            m_viewedAddresses.clear();
            index = 0;
        }
        if (index == m_viewedAddresses.size())
        {
            m_viewedAddresses.push_back(a);
        }
        ++index;

        if (a >= m_address) break;
        a = disassemble(d, a);
    }
    
    // The bar isn't on this view, so reset address
    if (row == m_height-1)
    {
        setView(m_address);
        m_topAddress = m_address;
    }
    else while (row > m_height / 2)
    {
        // We need to adjust the bar
        m_topAddress = disassemble(d, m_topAddress);
        if (index == m_viewedAddresses.size())
        {
            m_viewedAddresses.push_back(a);
        }
        ++index;

        a = disassemble(d, a);
        --row;
    }
}

int DisassemblyWindow::findViewAddress(u16 address)
{
    auto it = std::find(m_viewedAddresses.begin(), m_viewedAddresses.end(), address);
    return (it == m_viewedAddresses.end()) ? -1 : it - m_viewedAddresses.begin();
}

void DisassemblyWindow::setView(u16 newTopAddress)
{
    if (-1 == findViewAddress(newTopAddress))
    {
        m_viewedAddresses.clear();
    }
    m_topAddress = newTopAddress;
}

void DisassemblyWindow::cursorDown()
{
    int index = findViewAddress(m_address);
    assert(index != -1);
    
    Disassembler d;
    u16 nextAddress = disassemble(d, m_address);

    if (index == m_viewedAddresses.size() - 1)
    {
        // This is the last known address of the viewed addresses.  Add the new one to the end
        m_viewedAddresses.push_back(nextAddress);
    }
    else
    {
        // Check that the next address is what is expect, otherwise the following address are invalid.
        if (m_viewedAddresses[index+1] != nextAddress)
        {
            m_viewedAddresses.erase(m_viewedAddresses.begin() + index + 1, m_viewedAddresses.end());
            m_viewedAddresses.push_back(nextAddress);
        }
    }
    
    m_address = nextAddress;
}

void DisassemblyWindow::cursorUp()
{
    int index = findViewAddress(m_address);
    
    if (index == 0 || index == -1)
    {
        // We don't know the previous address.  Keep going back one byte until the disassembly of the instruction
        // finished up on the current address.
        u16 prevAddress = backInstruction(m_address);
        m_viewedAddresses.insert(m_viewedAddresses.begin(), prevAddress);
        m_address = prevAddress;
    }
    else
    {
        m_address = m_viewedAddresses[index - 1];
    }
}

void DisassemblyWindow::setCursor(u16 address)
{
    m_address = address;
    adjustBar();
}

void DisassemblyWindow::onDraw(Ui::Draw& draw)
{
    Disassembler d;
    u16 a = m_topAddress;
    u8 selectColour = draw.attr(Colour::Black, Colour::Yellow, true);
    u8 breakpointColour = draw.attr(Colour::Yellow, Colour::Red, true);
    u8 pcColour = draw.attr(Colour::White, Colour::Green, true);

    u8 bkg2 = m_bkgColour & ~0x40;
    for (int row = 1; row < m_height - 1; ++row)
    {
        u16 next = disassemble(d, a);
        u8 colour = (a == m_address)
            ? selectColour
            : (a == m_machine.getZ80().PC())
                ? pcColour
                : m_machine.hasUserBreakpointAt(a)
                    ? breakpointColour
                    : row & 1
                        ? m_bkgColour
                        : bkg2;

        draw.attrRect(m_x, m_y + row, m_width, 1, colour);
        draw.printString(m_x + 2, m_y + row, d.addressAndBytes(a).c_str(), colour);
        draw.printString(m_x + 21, m_y + row, d.opCode().c_str(), colour);
        draw.printString(m_x + 26, m_y + row, d.operands().c_str(), colour);
        
        if (m_machine.hasUserBreakpointAt(a))
        {
            draw.printChar(m_x + 1, m_y + row, ')', colour, gGfxFont);
        }
        a = next;
    }

}

void DisassemblyWindow::onKey(UiKey key, bool down)
{
    if (down)
    {
        using DK = UiKey;
        switch (key)
        {
        case DK::Up:
                cursorUp();
            adjustBar();
            break;

        case DK::Down:
            cursorDown();
            adjustBar();
            break;

        case DK::PageUp:
            for (int i = 0; i < (m_height - 2); ++i)
            {
                cursorUp();
            }
            adjustBar();
            break;

        case DK::PageDn:
            for (int i = 0; i < (m_height - 2); ++i)
            {
                cursorDown();
            }
            adjustBar();
            break;
                
        case DK::F9:
            m_machine.toggleBreakpoint(m_address);
            break;
            
        default:
            break;
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
// CPU Status window
//----------------------------------------------------------------------------------------------------------------------

CpuStatusWindow::CpuStatusWindow(Machine& M)
    : Window(M, 45, 1, 34, 20, "CPU Status", Colour::Black, Colour::White, false)
    , m_z80(M.getZ80())
{
    
}

void CpuStatusWindow::onDraw(Ui::Draw &draw)
{
    // Print out all the titles (in blue)
    u8 colour = draw.attr(Colour::Blue, Colour::White, false);
    
    draw.printString(m_x + 2, m_y + 1, "PC   AF   BC   DE   HL", colour);
    draw.printString(m_x + 2, m_y + 5, "SP   IX   IY   IR   WZ", colour);
    draw.printString(m_x + 3, m_y + 8, "T    S Z 5 H 3 V N C", colour);
    draw.printString(m_x + 1, m_y + 11, "IFF1", colour);
    draw.printString(m_x + 1, m_y + 12, "IFF2", colour);
    draw.printString(m_x + 1, m_y + 13, "IM", colour);
    draw.printString(m_x + 1, m_y + 14, "HALT", colour);

    draw.printSquashedString(m_x + 27, m_y + 1, "Stack", colour);
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) draw.printChar(m_x + 27, m_y + 3 + i, hex[i], colour);

    // Print out the registers
    colour = draw.attr(Colour::Black, Colour::White, false);
    printFormat(draw, 1, 2, colour, "%04X %04X %04X %04X %04X",
                m_z80.PC(), m_z80.AF(), m_z80.BC(), m_z80.DE(), m_z80.HL());
    printFormat(draw, 6, 3, colour, "%04X %04X %04X %04X",
                m_z80.AF_(), m_z80.BC_(), m_z80.DE_(), m_z80.HL_());
    printFormat(draw, 1, 6, colour, "%04X %04X %04X %04X %04X",
                m_z80.SP(), m_z80.IX(), m_z80.IY(), m_z80.IR(), m_z80.MP());
    printFormat(draw, 1, 9, colour, "%05d", (int)m_machine.getTState());
    
    // Print out the flags
    u8 f = m_z80.F();
    u8 con = draw.attr(Colour::Black, Colour::Green, true);
    u8 coff = draw.attr(Colour::Black, Colour::Red, true);
    for (int i = 0; i < 8; ++i)
    {
        draw.printChar(m_x + 8 + (i * 2), m_y + 9, (f & 0x80) ? '1' : '0',
                       (f & 0x80) ? con : coff);
        f <<= 1;
    }
    
    // Print out the interrupt status
    draw.printString(m_x + 7, m_y + 11, m_z80.IFF1() ? "On" : "Off", colour);
    draw.printString(m_x + 7, m_y + 12, m_z80.IFF2() ? "On" : "Off", colour);
    draw.printString(m_x + 7, m_y + 14, m_z80.isHalted() ? "Yes" : "No", colour);
    
    printFormat(draw, 7, 13, colour, "%d", m_z80.IM());
    
    // Print out the stack
    for (int i = 1; i < m_height - 1; ++i) draw.printChar(m_x + 26, m_y + i, '\'', colour, gGfxFont);
    draw.printChar(m_x + 26, m_y + m_height - 1, '(', colour, gGfxFont);

    u16 a = m_z80.SP();
    i64 ts = 0;
    for (int i = 0; i < 16; ++i)
    {
        printFormat(draw, 29, 3 + i, colour, "%04X", m_machine.getMemory().peek16(a, ts));
        a += 2;
    }
}

void CpuStatusWindow::onKey(UiKey key, bool down)
{
    
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL






















































