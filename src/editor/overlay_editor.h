//----------------------------------------------------------------------------------------------------------------------
// Editor Overlay
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <editor/editor.h>
#include <editor/window_editor.h>
#include <utils/ui.h>

#include <string>
#include <vector>

class EditorOverlay final : public Overlay
{
public:
    EditorOverlay(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

    EditorWindow& getWindow() { return m_window; }

private:
    EditorWindow    m_window;
    vector<string>  m_commands;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
