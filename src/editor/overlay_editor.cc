//----------------------------------------------------------------------------------------------------------------------
// Editor/Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <editor/overlay_editor.h>
#include <emulator/nx.h>

//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

EditorOverlay::EditorOverlay(Nx& nx)
    : Overlay(nx)
    , m_window(nx, "Editor/Assembler")
    , m_commands({
        "ESC|Exit",
        "Ctrl-S|Save",
        "Ctrl-O|Open",
        "Shift-Ctrl-S|Save as",
        "Ctrl-Tab|Switch buffers",
        })
{
    m_window.getEditor().getData().setTabs({ 8, 14, 32 }, 4);
}

void EditorOverlay::render(Draw& draw)
{
    m_window.draw(draw);
}

void EditorOverlay::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
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
}

void EditorOverlay::text(char ch)
{
    m_window.text(ch);
}

const vector<string>& EditorOverlay::commands() const
{
    return m_commands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
