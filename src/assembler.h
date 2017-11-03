//----------------------------------------------------------------------------------------------------------------------
// Assembler Overlay
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "ui.h"
#include "editor.h"

#include <vector>
#include <string>

class Assembler final : public Overlay
{
public:
    Assembler(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

private:
    EditorWindow    m_window;
    vector<string>  m_commands;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
