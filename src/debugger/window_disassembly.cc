//----------------------------------------------------------------------------------------------------------------------
// Disassembly Window
//----------------------------------------------------------------------------------------------------------------------

#include <asm/disasm.h>
#include <debugger/overlay_debugger.h>
#include <emulator/nx.h>

#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// Disassembler class
//----------------------------------------------------------------------------------------------------------------------


DisassemblyWindow::DisassemblyWindow(Nx& nx)
    : SelectableWindow(nx, 1, 22, 43, 30, "Disassembly", Colour::Black, Colour::White)
    , m_address(0x0000)
    , m_topAddress(0x0000)
    , m_gotoEditor(6, 23, 37, 1, Draw::attr(Colour::White, Colour::Magenta, false), false, 40, 0)
    , m_enableGoto(0)
{
    adjustBar();
    m_gotoEditor.onlyAllowHex();
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
    u16 pc = m_nx.getSpeccy().getZ80().PC();

    u8 bkg2 = m_bkgColour & ~0x40;
    for (int row = 1; row < m_height - 1; ++row)
    {
        u16 next = disassemble(d, a);
        u8 colour = (a == m_address)
            ? selectColour
            : (a == pc)
                ? pcColour
                : m_nx.getSpeccy().hasUserBreakpointAt(a)
                    ? breakpointColour
                    : row & 1
                        ? m_bkgColour
                        : bkg2;

        draw.attrRect(m_x, m_y + row, m_width, 1, colour);
        draw.printString(m_x + 2, m_y + row, d.addressAndBytes(a).c_str(), false, colour);
        draw.printString(m_x + 21, m_y + row, d.opCode().c_str(), false, colour);
        draw.printString(m_x + 26, m_y + row, d.operands().c_str(), false, colour);

        if (a != pc && m_nx.getSpeccy().hasUserBreakpointAt(a))
        {
            draw.printChar(m_x + 1, m_y + row, ')', colour, gGfxFont);
        }
        if (a == pc)
        {
            draw.printChar(m_x + 1, m_y + row, '*', colour, gGfxFont);
        }

        a = next;
    }

    if (m_enableGoto)
    {
        draw.attrRect(m_x, m_y + 1, m_width, 1, draw.attr(Colour::Black, Colour::Magenta, true));
        draw.printString(m_x + 1, m_y + 1, "    ", false, draw.attr(Colour::White, Colour::Magenta, true));
        draw.printSquashedString(m_x + 1, m_y + 1, "Goto:", draw.attr(Colour::Yellow, Colour::Magenta, true));
        m_gotoEditor.render(draw, 0);
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
            m_nx.getSpeccy().toggleBreakpoint(m_address);
            break;

        case K::G:
            m_gotoEditor.clear();
            m_enableGoto = 1;
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
            m_nx.getSpeccy().addTemporaryBreakpoint(m_address);
            m_nx.setRunMode(RunMode::Normal);
            if (m_nx.getRunMode() == RunMode::Stopped) m_nx.togglePause(false);
            break;

        default:
            break;
        }
    }

    if (m_enableGoto)
    {
        m_gotoEditor.key(key, down, shift, ctrl, alt);
    }
}

void DisassemblyWindow::onText(char ch)
{
    if (m_enableGoto == 0) return;
    if (m_enableGoto == 1)
    {
        // We swallow the first event, because it will be the key that enabled the goto.
        m_gotoEditor.clear();
        m_enableGoto = 2;
        return;
    }
    
    if (m_enableGoto)
    {
        switch(ch)
        {
            case 10:
            case 13:
            {
                m_enableGoto = 0;
                u16 t = 0;
                auto view = m_gotoEditor.getText();
                int len = view.size();
                if (len == 0)
                {
                    t = m_nx.getSpeccy().getZ80().PC();
                }
                else
                {
                    for (int i = 0; i < len; ++i)
                    {
                        t *= 16;
                        char c = view[i];
                        if (c >= '0' && c <= '9') t += (c - '0');
                        else if (c >= 'a' && c <= 'f') t += (c - 'a' + 10);
                        else if (c >= 'A' && c <= 'F') t += (c - 'A' + 10);
                    }
                }
                
                setCursor(t);
            }
            break;
                
            default:
                m_gotoEditor.text(ch);
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
    m_enableGoto = 0;
}
