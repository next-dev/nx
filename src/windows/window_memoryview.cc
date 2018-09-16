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
    , m_editAddress(0)
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
    return false;
}

//----------------------------------------------------------------------------------------------------------------------

void MemoryViewWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
