//----------------------------------------------------------------------------------------------------------------------
// Disassembly overlay
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <disasm/disassembler.h>
#include <utils/ui.h>

//----------------------------------------------------------------------------------------------------------------------
// Disassembler window
//----------------------------------------------------------------------------------------------------------------------

class Nx;

class DisassemblerWindow final : public Window
{
public:
    DisassemblerWindow(Nx& nx);

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

private:
    
};

//----------------------------------------------------------------------------------------------------------------------
// Disassembler overlay
//----------------------------------------------------------------------------------------------------------------------

class DisassemblerOverlay final : public Overlay
{
public:
    DisassemblerOverlay(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

    DisassemblerWindow& getWindow() { return m_window; }

private:
    DisassemblerWindow  m_window;
    vector<string>      m_commands;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
