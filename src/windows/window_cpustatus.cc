//----------------------------------------------------------------------------------------------------------------------
//! @file       windows/window_cpustatus.cc
//! @author     Matt Davies
//! @copyright  Copyright (C)2018, all rights reserved.
//! @brief      Implements the CPU status window

#include <emulator/nx.h>
#include <ui/draw.h>
#include <windows/window_cpustatus.h>

//----------------------------------------------------------------------------------------------------------------------

CpuStatusWindow::CpuStatusWindow(Nx& nx)
    : Window(nx)
{

}

//----------------------------------------------------------------------------------------------------------------------

void CpuStatusWindow::onRender(Draw& draw)
{
    Z80& z80 = getEmulator().getSpeccy().getCpu();

    //
    // Print out all the titles (in blue).
    //
    u8 colour = draw.attr(Colour::Blue, Colour::White);
    draw.printString(1, 0, "PC   AF   BC   DE   HL", colour);
    draw.printString(1, 4, "SP   IX   IY   IR   WZ", colour);
    draw.printString(2,  7, "T    S Z 5 H 3 V N C", colour);
    draw.printString(0,  15, "IFF1", colour);
    draw.printString(0,  16, "IFF2", colour);
    draw.printString(0,  17, "IM", colour);
    draw.printString(11, 15, "HALT", colour);
    draw.printString(11, 16, "FPS", colour);
    draw.printString(0,  10, "S0: ", colour);
    draw.printString(0,  11, "S1: ", colour);
    draw.printString(0,  12, "S2: ", colour);
    draw.printString(0,  13, "S3: ", colour);
    draw.printString(11, 10, "S4: ", colour);
    draw.printString(11, 11, "S5: ", colour);
    draw.printString(11, 12, "S6: ", colour);
    draw.printString(11, 13, "S7: ", colour);

    //
    // Draw stack titles
    //
    draw.printPropString(26, 0, "Stack", colour);
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) draw.printCharAttr(26, 2 + i, hex[i], colour);

    //
    // Print out the registers
    //
    colour = draw.attr(Colour::Black, Colour::White);
    draw.printString(0, 1, draw.format("%04X %04X %04X %04X %04X",
        z80.PC(), z80.AF(), z80.BC(), z80.DE(), z80.HL()), colour);
    draw.printString(5, 2, draw.format("%04X %04X %04X %04X",
        z80.AF_(), z80.BC_(), z80.DE_(), z80.HL_()), colour);
    draw.printString(0, 5, draw.format("%04X %04X %04X %04X %04X",
        z80.SP(), z80.IX(), z80.IY(), z80.IR(), z80.MP()), colour);
    draw.printString(0, 8, draw.format("%05d", (int)getEmulator().getSpeccy().getTState()), colour);

    //
    // Print out the flags
    //
    u8 f = z80.F();
    u8 con = draw.attr(Colour::Black, Colour::BrightGreen);
    u8 coff = draw.attr(Colour::Black, Colour::BrightRed);
    for (int i = 0; i < 8; ++i)
    {
        draw.printCharAttr(7 + i * 2, 8, (f & 0x80) ? FC_FILLED_SQUARE : FC_SQUARE,
            (f & 0x80) ? con : coff);
        f <<= 1;
    }

    //
    // Print out the interrupt status
    //
    draw.printString(6, 15, z80.IFF1() ? "On" : "Off", colour);
    draw.printString(6, 16, z80.IFF2() ? "On" : "Off", colour);
    draw.printString(17, 15, z80.isHalted() ? "Yes" : "No", colour);
    draw.printString(6, 17, draw.format("%d", z80.IM()), colour);

    //
    // FPS
    //
    static sf::Clock clock;
    draw.printString(17, 16, draw.format("%d", (int)(sf::seconds(1) / clock.restart())), colour);

    //
    // Print out the stack
    //
    for (int i = 0; i < draw.getHeight(); ++i)
    {
        draw.printCharAttr(25, i, FC_VERTICAL_LINE, colour);
    }
    //draw.pushShrink(-1);
    //draw.printCharAttr(24, draw.getHeight() - 1, FC_UPSIDE_DOWN_T, colour);
    //draw.popBounds();

    u16 a = z80.SP();
    TState t = 0;
    for (int i = 0; i < 16; ++i)
    {
        draw.printString(28, 2 + i, draw.format("%04X", getEmulator().getSpeccy().peek16(a, t)), colour);
        a += 2;
    }

    //
    // 
}

//----------------------------------------------------------------------------------------------------------------------

bool CpuStatusWindow::onKey(const KeyEvent& kev)
{
    return false;
}

//----------------------------------------------------------------------------------------------------------------------

void CpuStatusWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
