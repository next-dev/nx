//----------------------------------------------------------------------------------------------------------------------
// Memory dump window implementation
//----------------------------------------------------------------------------------------------------------------------

#include "debugger.h"
#include "spectrum.h"

#include <sstream>
#include <iomanip>

//----------------------------------------------------------------------------------------------------------------------
// Memory dump class
//----------------------------------------------------------------------------------------------------------------------

MemoryDumpWindow::MemoryDumpWindow(Spectrum& speccy)
    : SelectableWindow(speccy, 1, 1, 43, 20, "Memory Viewer", Colour::Black, Colour::White)
    , m_address(0)
{

}

void MemoryDumpWindow::onDraw(Draw& draw)
{
    u16 a = m_address;
    for (int i = 1; i < m_height - 1; ++i, a += 8)
    {
        using namespace std;
        stringstream ss;
        ss << setfill('0') << setw(4) << hex << uppercase << a << " : ";
        for (int b = 0; b < 8; ++b)
        {
            ss << setfill('0') << setw(2) << hex << uppercase << (int)m_speccy.peek(a + b) << " ";
        }
        ss << "  ";
        for (int b = 0; b < 8; ++b)
        {
            char ch = m_speccy.peek(a + b);
            ss << ((ch < 32 || ch > 127) ? '.' : ch);
        }
        draw.printString(m_x + 1, m_y + i, ss.str(), m_bkgColour);
    }
}

void MemoryDumpWindow::onKey(sf::Keyboard::Key key)
{
    using K = sf::Keyboard::Key;
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

    default:
        break;
    }
}

