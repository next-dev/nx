//----------------------------------------------------------------------------------------------------------------------
//! @file       windows/window_memoryview.cc
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Implements the memory view window

#include <emulator/nx.h>
#include <ui/draw.h>
#include <windows/window_memoryview.h>

//----------------------------------------------------------------------------------------------------------------------

MemoryViewWindow::MemoryViewWindow(Nx& nx)
    : Window(nx)
    , m_address(0x8000)
    , m_showChecksums(false)
    , m_editMode(false)
    , m_editAddress(0x8000)
    , m_editNibble(0)
{

}

//----------------------------------------------------------------------------------------------------------------------

void MemoryViewWindow::onRender(Draw& draw)
{
    u16 a = m_address;
    u8 bkg = draw.attr(getState().ink, getState().paper);
    int findIndex = 0;
    int findWidth = 0;
    u8 findColour = draw.attr(Colour::Black, Colour::BrightGreen);
    // #todo: link with find addresses
    const vector<u16> searches;

    for (int i = 0; i < draw.getHeight(); ++i, a += 8)
    {
        // The cursor draw position (to be calculated later).
        int cx = 0, cy = 0;

        //
        // Draw the address
        //
        string s = hexWord(a) + ": ";
        draw.printString(0, i, s, bkg);

        //
        // Hex digits
        //
        u16 ta(a);

        // Adjust to the next address we should look for (i.e. skip all addresses before this rendering point).
        while (
            (findIndex < (int)searches.size()) &&
            (ta > searches[findIndex]))
        {
            ++findIndex;
        }

        u16 checksum = 0;

        for (int b = 0; b < 8; ++b)
        {
            int dx = 6 + (b * 3);

            // Check for search element
            if (findIndex < (int)searches.size() && (ta + b) == searches[findIndex])
            {
                // Look out for next address;
                ++findIndex;
                // #todo: set this to the actual find width (number of consecutive bytes we're searching for).
                findWidth = 1;
            }

            // Check for cursor position
            if (!isPrompting() && m_editMode && (a + b) == m_editAddress)
            {
                cx = dx + m_editNibble;
                cy = i;
            }
            u8 x = getEmulator().getSpeccy().peek(a + b);
            checksum += x;

            u8 colour = findWidth > 0 ? findColour : bkg;
            s = hexByte(x);
            draw.printString(dx, i, s, colour);
            if (!m_showChecksums)
            {
                // Draw the character colours if we're not showing checksums.
                draw.pokeAttr(30 + b, i, colour);
            }
            // Set the colour for the space after the hex digits.  If the search term straddles two lines, we
            // want the highlight colour to stretch past the end.
            colour = (findWidth > 1 && b != 7) ? findColour : bkg;
            draw.printCharAttr(dx + 2, i, ' ', colour);

            if (findWidth > 0) --findWidth;
        }

        //
        // ASCII characters or checksums
        //
        if (m_showChecksums)
        {
            s = "= " + intString(checksum, 0);
            draw.printString(31, i, s, bkg);
        }
        else
        {
            for (int b = 0; b < 8; ++b)
            {
                char ch = getEmulator().getSpeccy().peek(a + b);
                draw.printChar(30 + b, i, (ch < 32 || ch > 127) ? '.' : ch);
            }
        }

        if (cx != 0)
        {
            // Draw the cursor
            draw.pokeAttr(cx, cy, draw.attr(Colour::White, Colour::BrightBlue));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

bool MemoryViewWindow::onKey(const KeyEvent& kev)
{
    using K = sf::Keyboard::Key;
    if (kev.isNormal())
    {
        if (m_editMode)
        {
            switch (kev.key)
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
                m_address -= (getState().height - 2) * 8;
                adjust();
                break;

            case K::PageDown:
                m_address += (getState().height - 2) * 8;
                adjust();
                break;

            case K::G:
                prompt("Goto", {}, [this](string text) {
                    jumpToAddress(text);
                }, RequireInputState::No);
                break;

            default:
                return false;
            }
        }
        else
        {
            switch (kev.key)
            {
            case K::Up:
                m_address -= 8;
                break;

            case K::Down:
                m_address += 8;
                break;

            case K::PageUp:
                m_address -= (getState().height - 2) * 8;
                break;

            case K::PageDown:
                m_address += (getState().height - 2) * 8;
                break;

            case K::G:
                prompt("Goto", {}, [this](string text) {
                    jumpToAddress(text);
                }, RequireInputState::No);
                break;

            case K::C:
                m_showChecksums = !m_showChecksums;
                break;

            case K::E:
                m_editMode = true;
                m_editAddress = m_address;
                adjust();
                break;

            default:
                return false;
            }
        }

        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------

void MemoryViewWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------

void MemoryViewWindow::adjust()
{
    // If the editing address is in the top half, do nothing.
    // If the editing address is in the bottom half, adjust view to make in centred.
    // If the editing address is not in the view, adjust the view to make it on the top line.

    u16 top = m_address;
    u16 end = m_address + (getState().height - 2) * 8;
    u16 mid = m_address + (getState().height / 2 - 1) * 8;
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
        m_address = (m_editAddress - ((getState().height / 2 - 1) * 8)) - adj;
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void MemoryViewWindow::poke(u8 value)
{
    u8 mask = m_editNibble ? 0xf0 : 0x0f;
    if (!m_editNibble) value <<= 4;
    Spectrum& speccy = getEmulator().getSpeccy();

    TState t;
    speccy.poke(m_editAddress, (speccy.peek(m_editAddress) & mask) | value, t);

    ++m_editNibble;
    if (m_editNibble == 2)
    {
        m_editNibble = 0;
        ++m_editAddress;
        adjust();
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
