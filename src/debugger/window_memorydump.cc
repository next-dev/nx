//----------------------------------------------------------------------------------------------------------------------
// Memory dump window implementation
//----------------------------------------------------------------------------------------------------------------------

#include <debugger/overlay_debugger.h>
#include <emulator/nx.h>

#include <iomanip>
#include <sstream>

//----------------------------------------------------------------------------------------------------------------------
// Memory dump class
//----------------------------------------------------------------------------------------------------------------------

MemoryDumpWindow::MemoryDumpWindow(Nx& nx)
    : SelectableWindow(nx, 1, 1, 43, 20, "Memory Viewer", Colour::Black, Colour::White)
    , m_address(0x8000)
    , m_gotoEditor(6, 2, 37, 1, Draw::attr(Colour::White, Colour::Magenta, false), false, 40, 0)
    , m_enableGoto(0)
    , m_showChecksums(0)
    , m_editMode(false)
    , m_editAddress(0)
    , m_editNibble(0)
{
}

void MemoryDumpWindow::zoomMode(bool flag)
{
    setPosition(1, 1, 43, flag ? 51 : 20);
}

void MemoryDumpWindow::onDraw(Draw& draw)
{
    u16 a = m_address;
    for (int i = 1; i < m_height - 1; ++i, a += 8)
    {
        int cx = 0, cy = 0;
        using namespace std;
        stringstream ss;
        ss << setfill('0') << setw(4) << hex << uppercase << a << " : ";
        u16 t = 0;
        for (int b = 0; b < 8; ++b)
        {
            // Check for cursor position
            if (!m_enableGoto && m_editMode && (a + b) == m_editAddress)
            {
                cx = m_x + 8 + (b * 3) + m_editNibble;
                cy = m_y + i;
            }
            u8 x = m_nx.getSpeccy().peek(a + b);
            t += x;
            ss << setfill('0') << setw(2) << hex << uppercase << (int)x << " ";
        }
        ss << "  ";
        if (m_showChecksums)
        {
            ss << "= " << dec << (int)t;
        }
        else
        {
            for (int b = 0; b < 8; ++b)
            {
                char ch = m_nx.getSpeccy().peek(a + b);
                ss << ((ch < 32 || ch > 127) ? '.' : ch);
            }
        }
        draw.printString(m_x + 1, m_y + i, ss.str(), false, m_bkgColour);
        if (cx != 0)
        {
            draw.pokeAttr(cx, cy, Draw::attr(Colour::White, Colour::Blue, true) | 0x80);

        }
    }

    if (m_enableGoto)
    {
        draw.attrRect(m_x, m_y + 1, m_width, 1, draw.attr(Colour::Black, Colour::Magenta, true));
        draw.printString(m_x + 1, m_y + 1, "    ", false, draw.attr(Colour::White, Colour::Magenta, true));
        draw.printSquashedString(m_x + 1, m_y + 1, "Goto:", draw.attr(Colour::Yellow, Colour::Magenta, true));
        m_gotoEditor.render(draw, 0);
    }
}

void MemoryDumpWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    if (!m_enableGoto || !m_gotoEditor.key(key, down, shift, ctrl, alt) && !down)
    {
        // An editor didn't handle the key so handle it here
        using K = sf::Keyboard::Key;
        if (!shift && !ctrl && !alt)
        {
            if (m_editMode)
            {
                switch (key)
                {
                case K::Escape:
                    m_editMode = false;
                    break;

                case K::Num0:   poke(0);    break;
                case K::Num1:   poke(1);    break;
                case K::Num2:   poke(2);    break;
                case K::Num3:   poke(3);    break;
                case K::Num4:   poke(4);    break;
                case K::Num5:   poke(5);    break;
                case K::Num6:   poke(6);    break;
                case K::Num7:   poke(7);    break;
                case K::Num8:   poke(8);    break;
                case K::Num9:   poke(9);    break;
                case K::A:      poke(10);   break;
                case K::B:      poke(11);   break;
                case K::C:      poke(12);   break;
                case K::D:      poke(13);   break;
                case K::E:      poke(14);   break;
                case K::F:      poke(15);   break;

                case K::Up:
                    m_editAddress -= 8;
                    adjust();
                    break;

                case K::Down:
                    m_editAddress += 8;
                    adjust();
                    break;

                case K::Left:
                    m_editAddress -= m_editNibble ? 0 : 1;
                    m_editNibble = !m_editNibble;
                    adjust();
                    break;

                case K::Right:
                    m_editAddress += m_editNibble ? 1 : 0;
                    m_editNibble = !m_editNibble;
                    adjust();
                    break;

                case K::PageUp:
                    m_address -= (m_height - 2) * 8;
                    adjust();
                    break;

                case K::PageDown:
                    m_address += (m_height - 2) * 8;
                    adjust();
                    break;

                case K::G:
                    m_gotoEditor.clear();
                    m_enableGoto = 1;
                    break;

                default:
                    break;
                }
            }
            else
            {
                switch (key)
                {
                case K::Up:
                    m_address -= 8;
                    break;

                case K::Down:
                    m_address += 8;
                    break;

                case K::PageUp:
                    m_address -= (m_height - 2) * 8;
                    break;

                case K::PageDown:
                    m_address += (m_height - 2) * 8;
                    break;

                case K::Escape:
                    m_enableGoto = false;
                    break;

                case K::G:
                    if (m_enableGoto == 0)
                    {
                        m_gotoEditor.clear();
                        m_enableGoto = 1;
                    }
                    break;

                case K::C:
                    m_showChecksums = !m_showChecksums;
                    break;

                case K::E:
                    if (m_enableGoto == 0)
                    {
                        m_editMode = true;
                        m_editAddress = m_address;
                        adjust();
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }

    if (m_enableGoto)
    {
        m_gotoEditor.key(key, down, shift, ctrl, alt);
    }
}

void MemoryDumpWindow::onUnselected()
{
    m_enableGoto = 0;
    m_editMode = 0;
}

void MemoryDumpWindow::onText(char ch)
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

                vector<u8> exprData = m_gotoEditor.getData().getData();
                if (exprData.size() == 0)
                {
                    m_editAddress = m_address = m_nx.getSpeccy().getZ80().PC();
                    m_editNibble = 0;
                }
                else if (optional<i64> result = m_nx.getAssembler().calculateExpression(exprData); result)
                {
                    m_editAddress = m_address = u16(*result);
                    m_editNibble = 0;
                }
                else
                {
                    m_nx.getDebugger().error("Invalid expression entered.");
                }

            }
            break;

            case 27:
                m_enableGoto = 0;
                break;

            default:
                m_gotoEditor.text(ch);
        }
    }
}

void MemoryDumpWindow::adjust()
{
    // If the editing address is in the top half, do nothing.
    // If the editing address is in the bottom half, adjust view to make in centred.
    // If the editing address is not in the view, adjust the view to make it on the top line.

    u16 top = m_address;
    u16 end = m_address + (m_height - 2) * 8;
    u16 mid = m_address + (m_height / 2 - 1) * 8;
    u16 adj = u16(m_editAddress - m_address) % 8;

    // 0 = address is outside view
    // 1 = address is on top half
    // 2 = address is on bottom half
    int state = 0;

    if (end < top)
    {
        // We have a wrap:
        //
        // ----------+--------------
        //           |
        // ----------+--------------
        // top       0000           end
        //
        if (m_editAddress >= end && m_editAddress < top)
        {
            // Address is outside view
            state = 0;
        }
        else
        {
            if (mid > top)
            {
                // Configuration 1:
                //
                //  ----------+-----+------
                //            |     |
                //  ----------+-----+------
                //  top       M     0000   end
                //
                state = (m_editAddress < mid && m_editAddress >= top) ? 1 : 2;
            }
            else
            {
                // Configuration 2a:
                //        
                //  ----+-----+----------
                //      |     |
                //  ----+-----+----------
                //  top 0000  M          end
                //
                state = (m_editAddress >= mid && m_editAddress < top) ? 2 : 1;
            }
        }
    }
    else
    {
        // Configuration 3:
        //
        //  +----------+----------+
        //  |          |          |
        //  +----------+----------+
        //  top        M           end
        //
        if (m_editAddress < top || m_editAddress >= end) state = 0;
        else
        {
            state = (m_editAddress < mid) ? 1 : 2;
        }
    }

    switch (state)
    {
    case 0:
        m_address = m_editAddress - adj;
        break;

    case 1:
        break;

    case 2:
        m_address = (m_editAddress - ((m_height / 2 - 1) * 8)) - adj;
        break;
    }
}

void MemoryDumpWindow::poke(u8 value)
{
    u8 mask = m_editNibble ? 0xf0 : 0x0f;
    if (!m_editNibble) value <<= 4;
    Spectrum& speccy = m_nx.getSpeccy();

    speccy.poke(m_editAddress, (speccy.peek(m_editAddress) & mask) | value);

    ++m_editNibble;
    if (m_editNibble == 2)
    {
        m_editNibble = 0;
        ++m_editAddress;
        adjust();
    }
}
