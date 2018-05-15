//----------------------------------------------------------------------------------------------------------------------
// Assembler Overlay
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <utils/ui.h>

//----------------------------------------------------------------------------------------------------------------------
// Results window
//----------------------------------------------------------------------------------------------------------------------


class AssemblerWindow final : public Window
{
public:
    AssemblerWindow(Nx& nx);

    void clear();
    void output(const std::string& msg);

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

private:
    vector<string>      m_lines;
    int                 m_topLine;
    int                 m_offset;
    size_t              m_longestLine;
};

//----------------------------------------------------------------------------------------------------------------------
// Assembler results overlay
//----------------------------------------------------------------------------------------------------------------------

class AssemblerOverlay final : public Overlay
{
public:
    AssemblerOverlay(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

    AssemblerWindow& getWindow() { return m_window; }

private:
    AssemblerWindow     m_window;
    vector<string>      m_commands;
};

