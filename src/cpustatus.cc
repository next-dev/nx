//----------------------------------------------------------------------------------------------------------------------
// CPU Status Window
//----------------------------------------------------------------------------------------------------------------------

#include "debugger.h"
#include "spectrum.h"

//----------------------------------------------------------------------------------------------------------------------
// CPU Status window
//----------------------------------------------------------------------------------------------------------------------

CpuStatusWindow::CpuStatusWindow(Spectrum& speccy)
    : Window(speccy, 45, 1, 34, 20, "CPU Status", Colour::Black, Colour::White, false)
    , m_z80(speccy.getZ80())
{

}

void CpuStatusWindow::onDraw(Draw &draw)
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
    draw.printString(m_x + 1, m_y + 16, "FPS", colour);

    draw.printSquashedString(m_x + 27, m_y + 1, "Stack", colour);
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) draw.printChar(m_x + 27, m_y + 3 + i, hex[i], colour);

    // Print out the registers
    colour = draw.attr(Colour::Black, Colour::White, false);
    draw.printString(m_x + 1, m_y + 2, draw.format("%04X %04X %04X %04X %04X",
        m_z80.PC(), m_z80.AF(), m_z80.BC(), m_z80.DE(), m_z80.HL()), colour);
    draw.printString(m_x + 6, m_y + 3, draw.format("%04X %04X %04X %04X",
        m_z80.AF_(), m_z80.BC_(), m_z80.DE_(), m_z80.HL_()), colour);
    draw.printString(m_x + 1, m_y + 6, draw.format("%04X %04X %04X %04X %04X",
        m_z80.SP(), m_z80.IX(), m_z80.IY(), m_z80.IR(), m_z80.MP()), colour);
    draw.printString(m_x + 1, m_y + 9, draw.format("%05d", (int)m_speccy.getTState()), colour);

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

    draw.printString(m_x + 7, m_y + 13, draw.format("%d", m_z80.IM()), colour);

    // FPS
    static sf::Clock clock;
    draw.printString(m_x + 7, m_y + 16, draw.format("%d", (int)(sf::seconds(1) / clock.restart())), colour);

    // Print out the stack
    for (int i = 1; i < m_height - 1; ++i) draw.printChar(m_x + 26, m_y + i, '\'', colour, gGfxFont);
    draw.printChar(m_x + 26, m_y + m_height - 1, '(', colour, gGfxFont);

    u16 a = m_z80.SP();
    i64 ts = 0;
    for (int i = 0; i < 16; ++i)
    {
        draw.printString(m_x + 29, m_y + 3 + i, draw.format("%04X", m_speccy.peek16(a, ts)), colour);
        a += 2;
    }
}

void CpuStatusWindow::onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt)
{

}

