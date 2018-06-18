//----------------------------------------------------------------------------------------------------------------------
// Disassembly Window
//----------------------------------------------------------------------------------------------------------------------

#include <asm/disasm.h>
#include <debugger/overlay_debugger.h>
#include <emulator/nx.h>
#include <utils/format.h>

#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// Disassembler class
//----------------------------------------------------------------------------------------------------------------------


DisassemblyWindow::DisassemblyWindow(Nx& nx)
    : SelectableWindow(nx, 1, 22, 43, 30, "Disassembly (Alt-1)", Colour::Black, Colour::White)
    , m_topAddress(0x8000)
    , m_address(0x8000)
    , m_firstLabel(0)
{
    adjustBar();
}

void DisassemblyWindow::zoomMode(bool flag)
{
    if (flag)
    {
        setPosition(1, 22, 78, 30);
    }
    else
    {
        setPosition(1, 22, 43, 30);
    }
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

    for (row = 1; row < m_height - 1; ++row)
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
    if (row == m_height - 1)
    {
        setView(m_address);
    }
    else while (row > m_height / 2)
    {
        // We need to adjust the bar
        m_topAddress = disassemble(d, m_topAddress);
        m_firstLabel = 0;
        MemAddr ta = m_nx.getSpeccy().convertAddress(Z80MemAddr(m_topAddress));
        while (m_firstLabel < m_labels.size() && m_labels[m_firstLabel].second < ta) ++m_firstLabel;
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
    return (it == m_viewedAddresses.end()) ? -1 : (int)(it - m_viewedAddresses.begin());
}

void DisassemblyWindow::setView(u16 newTopAddress)
{
    if (-1 == findViewAddress(newTopAddress))
    {
        m_viewedAddresses.clear();
        m_viewedAddresses.push_back(newTopAddress);
    }
    m_topAddress = newTopAddress;
    m_firstLabel = 0;
    MemAddr ta = m_nx.getSpeccy().convertAddress(Z80MemAddr(m_topAddress));

    // Find the first label
    while (m_firstLabel < m_labels.size())
    {
        optional<int> d = m_nx.diffZ80Address(m_labels[m_firstLabel].second, ta);
        if (d && (*d >= 0)) break;
        ++m_firstLabel;
    }
}

void DisassemblyWindow::cursorDown()
{
    int index = findViewAddress(m_address);
    NX_ASSERT(index != -1);

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
        if (m_viewedAddresses[index + 1] != nextAddress)
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

void DisassemblyWindow::onDraw(Draw& draw)
{
    Disassembler d;
    u16 a = m_topAddress;
    u8 selectColour = draw.attr(Colour::Black, Colour::Yellow, true);
    u8 breakpointColour = draw.attr(Colour::Yellow, Colour::Red, true);
    u8 pcColour = draw.attr(Colour::White, Colour::Green, true);
    u8 labelColour = draw.attr(Colour::Black, Colour::Cyan, true);
    u16 pc = m_nx.getSpeccy().getZ80().PC();

    u8 bkg2 = m_bkgColour & ~0x40;
    int labelIndex = m_firstLabel;

    for (int row = 1; row < m_height - 1; ++row)
    {
        bool printLabel = false;

        // Check for a label first
        MemAddr ta = m_nx.getSpeccy().convertAddress(Z80MemAddr(a));
        if ((labelIndex < m_labels.size()) && (ta == m_labels[labelIndex].second))
        {
            printLabel = true;

            u8 colour = labelColour; // row & 1 ? m_bkgColour : bkg2;
            draw.attrRect(m_x, m_y + row, m_width, 1, colour);
            draw.printString(m_x + 1, m_y + row, stringFormat("{0}:", m_labels[labelIndex].first), false, colour);

            ++labelIndex;
        }
        else
        // Otherwise, it's an instruction
        {
            MemAddr ta = m_nx.getSpeccy().convertAddress(Z80MemAddr(a));
            u8 colour = (a == m_address)
                ? selectColour
                : (a == pc)
                    ? pcColour
                    : m_nx.getSpeccy().hasUserBreakpointAt(ta)
                        ? breakpointColour
                        : row & 1
                            ? m_bkgColour
                            : bkg2;
            u16 next = disassemble(d, a);

            draw.attrRect(m_x, m_y + row, m_width, 1, colour);
            draw.printString(m_x + 2, m_y + row, d.addressAndBytes(a).c_str(), false, colour);
            draw.printString(m_x + 21, m_y + row, d.opCodeString().c_str(), false, colour);
            draw.printString(m_x + 26, m_y + row, d.operandString().c_str(), false, colour);

            if (a != pc && m_nx.getSpeccy().hasUserBreakpointAt(ta))
            {
                draw.printChar(m_x + 1, m_y + row, ')', colour, gGfxFont);
            }
            if (a == pc)
            {
                draw.printChar(m_x + 1, m_y + row, '*', colour, gGfxFont);
            }

            a = next;
        }
    }
}

void DisassemblyWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;

    if (down && !shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::Up:
            cursorUp();
            adjustBar();
            break;

        case K::Down:
            cursorDown();
            adjustBar();
            break;

        case K::PageUp:
            for (int i = 0; i < (m_height - 2); ++i)
            {
                cursorUp();
            }
            adjustBar();
            break;

        case K::PageDown:
            for (int i = 0; i < (m_height - 2); ++i)
            {
                cursorDown();
            }
            adjustBar();
            break;

        case K::F9:
            {
                MemAddr ta = m_nx.getSpeccy().convertAddress(Z80MemAddr(m_address));
                m_nx.getSpeccy().toggleBreakpoint(ta);
            }
            break;

        case K::G:
            prompt("Goto", [this](string text)
            {
                if (optional<MemAddr> addr = m_nx.textToAddress(text); addr)
                {
                    Z80MemAddr z80Addr = m_nx.getSpeccy().convertAddress(*addr);
                    setCursor(z80Addr);
                }
            }, true);
            break;

        default:
            break;
        }
    }
    else if (down && !shift && ctrl && !alt)
    {
        switch (key)
        {
        case K::F5:
            // Run to
            {
                MemAddr ta = m_nx.getSpeccy().convertAddress(Z80MemAddr(m_address));
                m_nx.getSpeccy().addTemporaryBreakpoint(ta);
                m_nx.setRunMode(RunMode::Normal);
                if (m_nx.getRunMode() == RunMode::Stopped) m_nx.togglePause(false);
            }
            break;

        default:
            break;
        }
    }
}

void DisassemblyWindow::onText(char ch)
{
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
        m_nx.getSpeccy().peek(address + 0),
        m_nx.getSpeccy().peek(address + 1),
        m_nx.getSpeccy().peek(address + 2),
        m_nx.getSpeccy().peek(address + 3));
}

u16 DisassemblyWindow::disassemble(u16 address)
{
    Disassembler d;
    return disassemble(d, address);
}

void DisassemblyWindow::onUnselected()
{
    killPrompt();
}

