//----------------------------------------------------------------------------------------------------------------------
// Editor
//----------------------------------------------------------------------------------------------------------------------

#include <asm/overlay_asm.h>
#include <editor/overlay_editor.h>
#include <emulator/nx.h>

//----------------------------------------------------------------------------------------------------------------------
// EditorOverlay
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
        "Ctrl-B|Build",
        })
{
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
    else if (down && !shift && ctrl && !alt)
    {
        if (key == K::B)
        {
            // Ensure that all files are saved
            if (m_window.saveAll())
            {
                getEmulator().assemble(m_window.getEditor().getFileName());
            }
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
