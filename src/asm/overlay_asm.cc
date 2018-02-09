//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/overlay_asm.h>
#include <emulator/nx.h>
#include <utils/format.h>

#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// AssemblerWindow
//----------------------------------------------------------------------------------------------------------------------

AssemblerWindow::AssemblerWindow(Nx& nx)
    : Window(nx, 1, 1, 78, 60, "Assembler Results", Colour::Blue, Colour::Black, false)
    , m_topLine(0)
{
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
    u8 normal = colour;
    u8 error = draw.attr(Colour::Red, Colour::Black, false);
    u8 ok = draw.attr(Colour::Green, Colour::Black, false);

    enum class LineType
    {
        Normal,
        Error
    };

    for (; (y < endY) && (line < m_lines.size()); ++y, ++line)
    {
        colour = normal;
        const string& msg = m_lines[line];
        int i = 0;
        int x = m_x + 1;
        int endX = min(int(x + msg.size()), m_x + m_width - 1);
        LineType lineType = LineType::Normal;

        if (!msg.empty())
        {
            if (msg[0] == '!')
            {
                lineType = LineType::Error;
                ++i;
            }
            else if (msg[0] == '*')
            {
                colour = ok;
            }
        }

        while (x < endX)
        {
            draw.printChar(x++, y, msg[i], colour);
            if (lineType == LineType::Error)
            {
                if (msg[i] == ':' && msg[i-1] == ')') colour = error;
            }
            ++i;
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
    if (down && !shift && !ctrl && !alt)
    {
        using K = sf::Keyboard::Key;
        switch (key)
        {
        case K::Up:
            m_topLine = max(0, m_topLine - 1);
            break;

        case K::Down:
            m_topLine = max(0, min(m_topLine + 1, int(m_lines.size() - 1)));
            break;

        case K::PageUp:
            m_topLine = max(0, m_topLine - (m_height - 2));
            break;

        case K::PageDown:
            m_topLine = max(0, min(m_topLine + (m_height - 2), int(m_lines.size() - 1)));
            break;

        case K::Home:
            m_topLine = 0;
            break;

        case K::End:
            m_topLine = max(0, int(m_lines.size()) - (m_height/2 - 1));
            break;

        case K::Escape:
            m_nx.showEditor();
            break;
        }
    }
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
            "ESC|Exits",
            "Up/Down|Scroll",
            "PgUp/PgDn|Page",
            "Home|Top",
            "End|Bottom",
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
