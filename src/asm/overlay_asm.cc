//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/overlay_asm.h>

#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// AssemblerWindow
//----------------------------------------------------------------------------------------------------------------------

AssemblerWindow::AssemblerWindow(Nx& nx)
    : Window(nx, 1, 1, 78, 60, "Assembler Results", Colour::Blue, Colour::Black, false)
    , m_topLine(0)
{
    for (int i = 0; i < 16; ++i) output("Hello World!\n");
}

void AssemblerWindow::clear()
{
    m_lines.clear();
    m_topLine = 0;
}

void AssemblerWindow::output(const std::string& msg)
{
    m_lines.emplace_back(msg);
}

void AssemblerWindow::onDraw(Draw& draw)
{
    int line = m_topLine;
    int y = m_y + 1;
    int endY = m_y + m_height - 1;
    u8 colour = draw.attr(Colour::White, Colour::Black, false);

    for (; (y < endY) && (line < m_lines.size()); ++y, ++line)
    {
        const string& msg = m_lines[line];
        int i = 0;
        int x = m_x + 1;
        int endX = min(int(x + msg.size()), m_x + m_width - 1);
        while (x < endX)
        {
            draw.printChar(x++, y, msg[i++], colour);
        }
        while (x < (m_x + m_height - 1))
        {
            draw.printChar(x++, y, ' ', colour);
        }
    }
    while (y < endY)
    {
        for (int x = m_x; x < (m_x + m_width - 1); ++x)
        {
            draw.printChar(x, y, ' ', colour);
        }
        ++y;
    }
}

void AssemblerWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{

}

void AssemblerWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
// AssemblerOverlay
//----------------------------------------------------------------------------------------------------------------------

AssemblerOverlay::AssemblerOverlay(Nx& nx)
    : Overlay(nx)
    , m_window(nx)
    , m_commands({
            "Any Key|Exit results",
        })
{
}

void AssemblerOverlay::render(Draw& draw)
{
    m_window.draw(draw);
}

void AssemblerOverlay::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    m_window.keyPress(key, down, shift, ctrl, alt);
}

void AssemblerOverlay::text(char ch)
{
    m_window.text(ch);
}

const vector<string>& AssemblerOverlay::commands() const
{
    return m_commands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
