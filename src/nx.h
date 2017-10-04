//----------------------------------------------------------------------------------------------------------------------
// NX system
// Launches other subsystems based command line arguments, and manages memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "host.h"
#include "machine.h"
#include "ui.h"

enum class DebugKey
{
    Left,
    Down,
    Up,
    Right,
    PageUp,
    PageDn,

    COUNT
};

class Nx
{
public:
    Nx(IHost& host, u32* img, u32* ui_img);

    // Advance as many number of t-states as possible.  Returns the number of t-States that were processed.
    i64 update();

    //
    // Keyboard control
    //
    void keyPress(Key k, bool down);
    void keysClear();
    void debugKeyPress(DebugKey k, bool down);

    //
    // File loading
    //
    bool load(std::string fileName);

    //
    // Debugger
    //
    bool isDebugging() const { return m_debugger; }
    void drawDebugger(Ui::Draw& draw);
    void toggleDebugger();

    //
    // Overlay
    //
    void drawOverlay(Ui::Draw& draw);

private:
    //
    // Debugger
    //
    void drawMemDump(Ui::Draw& draw);

private:
    IHost&              m_host;
    i64                 m_tState;
    i64                 m_elapsedTStates;
    std::vector<bool>   m_keys;
    Machine             m_machine;
    Ui                  m_ui;
    bool                m_debugger;

    // Memory dump state
    u16                 m_address;

};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <functional>
#include <sstream>
#include <iomanip>

Nx::Nx(IHost& host, u32* img, u32* ui_img)
    : m_host(host)
    , m_tState(0)
    , m_elapsedTStates(0)
    , m_keys((int)Key::COUNT)
    , m_machine(host, img, m_keys)
    , m_ui(ui_img, m_machine.getMemory(), m_machine.getZ80(), m_machine.getIo())
    , m_debugger(false)
    //--- Memory dump state -------------------------------------------------------------
    , m_address(0)
{

}

i64 Nx::update()
{
    if (m_debugger)
    {
        m_ui.clear();
        m_ui.render(std::bind(&Nx::drawDebugger, this, std::placeholders::_1));
        return 0;
    }
    else
    {
        i64 elapsedTStates = m_machine.update(m_tState);
        m_elapsedTStates += elapsedTStates;
        if (m_elapsedTStates > 69888)
        {
            m_elapsedTStates -= 69888;
            m_ui.clear();
            m_ui.render(std::bind(&Nx::drawOverlay, this, std::placeholders::_1));
        }
        return elapsedTStates;
    }

}

void Nx::keyPress(Key k, bool down)
{
    m_keys[(int)k] = down;
}

void Nx::keysClear()
{
    for (auto& k : m_keys) k = 0;
}

bool Nx::load(std::string fileName)
{
    const u8* buffer;
    i64 size;

    int handle = m_host.load(fileName, buffer, size);
    if (handle)
    {
        bool result = m_machine.load(buffer, size, Machine::FileType::Sna, m_tState);
        m_host.unload(handle);

        return result;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Debugger
//----------------------------------------------------------------------------------------------------------------------

void Nx::drawDebugger(Ui::Draw& draw)
{
    drawMemDump(draw);
}

void Nx::toggleDebugger()
{
    m_debugger = !m_debugger;
}

void Nx::drawMemDump(Ui::Draw& draw)
{
    static const int kX = 1;
    static const int kY = 1;
    static const int kWidth = 43;
    static const int kHeight = 20;

    draw.window(kX, kY, kWidth, kHeight, "Memory View");

    u16 a = m_address;
    for (int i = 1; i < kHeight - 1; ++i, a += 8)
    {
        using namespace std;
        stringstream ss;
        ss << setfill('0') << setw(4) << hex << uppercase << a << " : ";
        for (int b = 0; b < 8; ++b)
        {
            ss << setfill('0') << setw(2) << hex << uppercase << (int)m_machine.getMemory().peek(a + b) << " ";
        }
        ss << "  ";
        for (int b = 0; b < 8; ++b)
        {
            char ch = m_machine.getMemory().peek(a + b);
            ss << ((ch < 32 || ch > 127) ? '.' : ch);
        }
        draw.printString(kX + 1, kY + i, ss.str().c_str(), 0xf8);
    }
}

void Nx::debugKeyPress(DebugKey k, bool down)
{
    if (down)
    {
        using DK = DebugKey;
        switch (k)
        {
        case DK::Up:
            m_address -= 8;
            break;

        case DK::Down:
            m_address += 8;
            break;

        case DK::PageUp:
            m_address -= 18 * 8;
            break;

        case DK::PageDn:
            m_address += 18 * 8;
            break;

        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Overlay
//----------------------------------------------------------------------------------------------------------------------

void Nx::drawOverlay(Ui::Draw& draw)
{

}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
