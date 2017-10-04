//----------------------------------------------------------------------------------------------------------------------
// NX system
// Launches other subsystems based command line arguments, and manages memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "host.h"
#include "machine.h"
#include "ui.h"

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

    //
    // File loading
    //
    bool load(std::string fileName);

    //
    // Debugger
    //
    void drawDebugger(Ui::Draw& draw);

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
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <functional>

Nx::Nx(IHost& host, u32* img, u32* ui_img)
    : m_host(host)
    , m_tState(0)
    , m_elapsedTStates(0)
    , m_keys((int)Key::COUNT)
    , m_machine(host, img, m_keys)
    , m_ui(ui_img, m_machine.getMemory(), m_machine.getZ80(), m_machine.getIo())
{

}

i64 Nx::update()
{
    i64 elapsedTStates = m_machine.update(m_tState);
    m_elapsedTStates += elapsedTStates;
    if (m_elapsedTStates >= 69888)
    {
        m_ui.clear();
        m_ui.render(std::bind(&Nx::drawDebugger, this, std::placeholders::_1));
        m_elapsedTStates -= 69888;
    }

    return elapsedTStates;
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
    draw.window(1, 1, 40, 20, "Research", draw.attr(Ui::Draw::Colour::Black, Ui::Draw::Colour::White, true));
    draw.printSquashedString(1, 50, "Testing...", 0xf8);
}

void Nx::drawMemDump(Ui::Draw& draw)
{

}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
