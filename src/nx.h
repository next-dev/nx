//----------------------------------------------------------------------------------------------------------------------
// NX system
// Launches other subsystems based command line arguments, and manages memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "host.h"
#include "machine.h"

class Nx
{
public:
    Nx(IHost& host, u32* img);

    // Advance as many number of t-states as possible.  Returns the number of t-States that were processed.
    i64 update();

    //
    // Keyboard control
    //
    void keyPress(Key k, bool down);
    void keysClear();

private:
    IHost&              m_host;
    i64                 m_tState;
    std::vector<bool>   m_keys;
    Machine             m_machine;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

Nx::Nx(IHost& host, u32* img)
    : m_host(host)
    , m_tState(0)
    , m_keys((int)Key::COUNT)
    , m_machine(host, img, m_keys)
{

}

i64 Nx::update()
{
    return m_machine.update(m_tState);
}

void Nx::keyPress(Key k, bool down)
{
    m_keys[(int)k] = down;
}

void Nx::keysClear()
{
    for (auto& k : m_keys) k = 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
