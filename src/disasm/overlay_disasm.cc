//----------------------------------------------------------------------------------------------------------------------
// Disassembler
//----------------------------------------------------------------------------------------------------------------------

#include <disasm/overlay_disasm.h>
#include <emulator/nx.h>

//----------------------------------------------------------------------------------------------------------------------
// DisassemblerWindow
//----------------------------------------------------------------------------------------------------------------------

DisassemblerWindow::DisassemblerWindow(Nx& nx)
    : Window(nx, 1, 1, 78, 59, "Disassembler", Colour::Blue, Colour::Black, false)
{

}

void DisassemblerWindow::onDraw(Draw& draw)
{
    string line1 = "Press {Ctrl-O} to open a disassembly session for editing";
    string line2 = "Press {Ctrl-N} to create a new disassembly session";
    u8 colour = draw.attr(Colour::White, Colour::Black, false);

    int y = m_y + (m_height / 2);

    draw.printString(m_x + (m_width - int(line1.size())) / 2, y - 1, line1, true, colour);
    draw.printString(m_x + (m_width - int(line2.size())) / 2, y + 1, line2, true, colour);
}

void DisassemblerWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{

}

void DisassemblerWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
// DisassemblerOverlay
//----------------------------------------------------------------------------------------------------------------------

DisassemblerOverlay::DisassemblerOverlay(Nx& nx)
    : Overlay(nx)
    , m_window(nx)
    , m_commands({
        "ESC|Exit",
        "Ctrl-S|Save",
        "Ctrl-O|Open",
        "Shift-Ctrl-S|Save as",
        "Ctrl-Tab|Switch buffers",
        "Ctrl-B|Build",
        })
{
}

void DisassemblerOverlay::render(Draw& draw)
{
    m_window.draw(draw);
}

void DisassemblerOverlay::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;

    m_window.keyPress(key, down, shift, ctrl, alt);

    if (down && !shift && !ctrl && !alt)
    {
        if (key == K::Escape)
        {
            getEmulator().hideAll();
        }
    }
    else if (down && !shift && ctrl && !alt)
    {
        bool buildSuccess = false;

        if (key == K::B)
        {
            // Generate the files.
        }
    }
}

void DisassemblerOverlay::text(char ch)
{
    m_window.text(ch);
}

const vector<string>& DisassemblerOverlay::commands() const
{
    return m_commands;
}
