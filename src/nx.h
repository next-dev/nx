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
    Tab,

    COUNT
};

enum class JoystickKey
{
    Left,
    Down,
    Up,
    Right,
    Fire,
};

class Disassembler;

class Nx
{
public:
    Nx(IHost& host, u32* img, u32* ui_img, int argc, char** argv);

    // Advance as many number of t-states as possible.  Returns the number of t-States that were processed.
    i64 update();

    //
    // Input control
    //
    void keyPress(Key k, bool down);
    void keysClear();
    void debugKeyPress(DebugKey k, bool down);
    void joystickKeyPress(JoystickKey k, bool down);

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

    //
    // Settings
    //
    std::string getSetting(std::string key, std::string defaultSetting);
    void setSetting(std::string key, std::string setting);

    bool usesKempstonJoystick() const { return m_kempstonJoystick; }

private:
    //
    // Debugger
    //
    void drawMemDump(Ui::Draw& draw);
    void drawDisassembler(Ui::Draw& draw);
    u16 disassemble(Disassembler& d, u16 address);
    u16 backInstruction(u16 address);

    //
    // Settings
    //
    void updateSettings();

private:
    IHost&              m_host;
    i64                 m_tState;
    i64                 m_elapsedTStates;
    std::vector<bool>   m_keys;
    Machine             m_machine;
    Ui                  m_ui;
    bool                m_debugger;

    // Window layout
    struct WindowPos
    {
        int x, y;
        int width, height;
    };
    WindowPos           m_memoryDumpWindow;
    WindowPos           m_dissasemblyWindow;
    WindowPos*          m_currentWindow;

    // Memory dump state
    u16                 m_address;

    // Disassembler state
    u16                 m_rootAddress;

    // Settings
    std::map<std::string, std::string>  m_settings;
    bool                                m_kempstonJoystick;     // Cursor keys & TAB mapped to Kempston joystick
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <functional>
#include <sstream>
#include <iomanip>

#include "disasm.h"

Nx::Nx(IHost& host, u32* img, u32* ui_img, int argc, char** argv)
    : m_host(host)
    , m_tState(0)
    , m_elapsedTStates(0)
    , m_keys((int)Key::COUNT)
    , m_machine(host, img, m_keys)
    , m_ui(ui_img, m_machine.getMemory(), m_machine.getZ80(), m_machine.getIo())
    , m_debugger(false)
    //--- Window layout -----------------------------------------------------------------
    , m_memoryDumpWindow({ 1, 1, 43, 20 })
    , m_dissasemblyWindow({ 1, 22, 43, 30})
    , m_currentWindow(&m_dissasemblyWindow)
    //--- Memory dump state -------------------------------------------------------------
    , m_address(0)
    //--- Disassemblye state ------------------------------------------------------------
    , m_rootAddress(0)
    //--- Settings ----------------------------------------------------------------------
    , m_kempstonJoystick(false)
{
    // Deal with the command line
    for (int i = 1; i < argc; ++i)
    {
        char* arg = argv[i];
        if (arg[0] == '-')
        {
            // Setting being added
            char* keyEnd = strchr(arg, '=');
            char* keyStart = arg + 1;
            if (keyEnd)
            {
                std::string key(keyStart, keyEnd);
                std::string value(keyEnd+1);
                setSetting(key, value);
            }
            else
            {
                // Assume key is "yes"
                setSetting(arg + 1, "yes");
            }
        }
        else
        {
            load(arg);
        }
    }

    updateSettings();
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

void Nx::joystickKeyPress(JoystickKey k, bool down)
{
    u8 bit = 0;

    switch (k)
    {
    case JoystickKey::Right:    bit = 0x01;     break;
    case JoystickKey::Left:     bit = 0x02;     break;
    case JoystickKey::Down:     bit = 0x04;     break;
    case JoystickKey::Up:       bit = 0x08;     break;
    case JoystickKey::Fire:     bit = 0x10;     break;
    }

    Io& io = m_machine.getIo();
    if (down)
    {
        io.setKempstonState(io.getKempstonState() | bit);
    }
    else
    {
        io.setKempstonState(io.getKempstonState() & ~bit);
    }
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
// Settings
//----------------------------------------------------------------------------------------------------------------------

std::string Nx::getSetting(std::string key, std::string defaultSetting)
{
    auto it = m_settings.find(key);
    return it == m_settings.end() ? defaultSetting : it->second;
}

void Nx::setSetting(std::string key, std::string setting)
{
    m_settings[key] = setting;
}

void Nx::updateSettings()
{
    std::string v;

    v = getSetting("kempston", "no");
    m_kempstonJoystick = (v == "yes");
}

//----------------------------------------------------------------------------------------------------------------------
// Debugger
//----------------------------------------------------------------------------------------------------------------------

void Nx::drawDebugger(Ui::Draw& draw)
{
    drawMemDump(draw);
    drawDisassembler(draw);
}

void Nx::toggleDebugger()
{
    m_debugger = !m_debugger;
}

void Nx::drawMemDump(Ui::Draw& draw)
{
    WindowPos& w = m_memoryDumpWindow;
    bool currentWindow = m_currentWindow == &m_memoryDumpWindow;
    u8 bkg = draw.attr(Ui::Draw::Colour::Black, Ui::Draw::Colour::White, currentWindow);

    draw.window(w.x, w.y, w.width, w.height, "Memory View", currentWindow, bkg);

    u16 a = m_address;
    for (int i = 1; i < w.height - 1; ++i, a += 8)
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
        draw.printString(w.x + 1, w.y + i, ss.str().c_str(), bkg);
    }
}

u16 Nx::backInstruction(u16 address)
{
    u16 count = 1;
    Disassembler d;

    // Keep disassembling back until the address after the instruction is equal
    // to the current one
    while (count < 4) {
        u16 a = address - count;
        a = disassemble(d, a);
        if (a == address)
        {
            // This instruction will do!
            return address - count;
        }
        ++count;
    }

    // Couldn't find a suitable instruction, fallback: just go back one byte
    return address - 1;
}

void Nx::debugKeyPress(DebugKey k, bool down)
{
    if (down)
    {
        using DK = DebugKey;
        switch (k)
        {
        case DK::Up:
            if (m_currentWindow == &m_memoryDumpWindow)
            {
                m_address -= 8;
            }
            else if (m_currentWindow == &m_dissasemblyWindow)
            {
                m_rootAddress = backInstruction(m_rootAddress);
            }
            break;

        case DK::Down:
            if (m_currentWindow == &m_memoryDumpWindow)
            {
                m_address += 8;
            }
            else if (m_currentWindow == &m_dissasemblyWindow)
            {
                Disassembler d;
                m_rootAddress = disassemble(d, m_rootAddress);
            }
            break;

        case DK::PageUp:
            if (m_currentWindow == &m_memoryDumpWindow)
            {
                m_address -= (m_currentWindow->height-2) * 8;
            }
            else if (m_currentWindow == &m_dissasemblyWindow)
            {
                for (int i = 0; i < (m_currentWindow->height - 2); ++i)
                {
                    m_rootAddress = backInstruction(m_rootAddress);
                }
            }
            break;

        case DK::PageDn:
            if (m_currentWindow == &m_memoryDumpWindow)
            {
                m_address += (m_currentWindow->height - 2) * 8;
            }
            else if (m_currentWindow == &m_dissasemblyWindow)
            {
                for (int i = 0; i < (m_currentWindow->height - 2); ++i)
                {
                    Disassembler d;
                    m_rootAddress = disassemble(d, m_rootAddress);
                }
            }
            break;

        case DK::Tab:
            if (down) m_currentWindow = (m_currentWindow == &m_memoryDumpWindow)
                ? &m_dissasemblyWindow : &m_memoryDumpWindow;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Disassembler
//----------------------------------------------------------------------------------------------------------------------

u16 Nx::disassemble(Disassembler& d, u16 address)
{
    return d.disassemble(address,
        m_machine.getMemory().peek(address + 0),
        m_machine.getMemory().peek(address + 1),
        m_machine.getMemory().peek(address + 2),
        m_machine.getMemory().peek(address + 3));
}

void Nx::drawDisassembler(Ui::Draw& draw)
{
    WindowPos& w = m_dissasemblyWindow;
    bool currentWindow = m_currentWindow == &m_dissasemblyWindow;
    u8 bkg = draw.attr(Ui::Draw::Colour::Black, Ui::Draw::Colour::White, currentWindow);
    draw.window(w.x, w.y, w.width, w.height, "Disassembly", currentWindow, bkg);

    Disassembler d;
    u16 a = m_rootAddress;

    for (int row = 1; row < w.height - 1; ++row)
    {
        u16 next = disassemble(d, a);

        draw.printString(w.x + 1, w.y + row, d.addressAndBytes(a).c_str(), bkg);
        draw.printString(w.x + 20, w.y + row, d.opCode().c_str(), bkg);
        draw.printString(w.x + 25, w.y + row, d.operands().c_str(), bkg);
        a = next;
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
