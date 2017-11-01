//----------------------------------------------------------------------------------------------------------------------
// Assembler Overlay
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "ui.h"
#include "editor.h"

#include <vector>

class Assembler final : public Overlay
{
public:
    Assembler(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    void const vector<string>& commands() const override;

private:
    EditorWindow    m_window;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
