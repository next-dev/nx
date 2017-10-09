//----------------------------------------------------------------------------------------------------------------------
// NX system
// Launches other subsystems based command line arguments, and manages memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "host.h"
#include "machine.h"
#include "ui.h"
#include "debugger.h"

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
    void update();
    
    // Debugger functions.  These only work if the run mode is current RunMode::Stopped.
    void stepOver();
    void stepIn();

    // Access to the underlying machine
    Machine& getMachine() { return m_machine; }

    // Restart the machine
    void restart();

    //
    // Input control
    //
    void keyPress(Key k, bool down);
    void keysClear();
    void uiKeyPress(UiKey k, bool down);
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
    void togglePause(bool wasBreakpoint);

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
    std::vector<bool>   m_keys;
    Machine             m_machine;
    Ui                  m_ui;
    bool                m_debugger;
    RunMode             m_runMode;

    // Debugger windows
    MemoryDumpWindow    m_memoryDumpWindow;
    DisassemblyWindow   m_dissasemblyWindow;
    CpuStatusWindow     m_cpuStatusWindow;

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

Nx::Nx(IHost& host, u32* img, u32* ui_img, int argc, char** argv)
    : m_host(host)
    , m_keys((int)Key::COUNT)
    , m_machine(host, img, m_keys)
    , m_ui(ui_img, m_machine.getMemory(), m_machine.getZ80(), m_machine.getIo())
    , m_debugger(false)
    , m_runMode(RunMode::Normal)
    //--- Windows
    , m_memoryDumpWindow(m_machine)
    , m_dissasemblyWindow(m_machine)
    , m_cpuStatusWindow(m_machine)
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

    m_dissasemblyWindow.Select();
}

void Nx::update()
{
    bool breakpointHit = false;
    m_machine.update(m_runMode, breakpointHit);
    if (breakpointHit)
    {
        m_dissasemblyWindow.setCursor(getMachine().getZ80().PC());
        togglePause(true);
    }
    m_ui.clear();
    m_ui.render(std::bind(m_debugger ? &Nx::drawDebugger : &Nx::drawOverlay, this, std::placeholders::_1));
    m_machine.getAudio().waitFrame();
}

void Nx::stepIn()
{
    assert(m_debugger);
    if (m_runMode == RunMode::Normal)
    {
        togglePause(false);
    }
    bool breakpointHit;
    m_machine.update(RunMode::StepIn, breakpointHit);
    m_dissasemblyWindow.setCursor(getMachine().getZ80().PC());
    m_ui.clear();
    m_ui.render(std::bind(&Nx::drawDebugger, this, std::placeholders::_1));
}

void Nx::stepOver()
{
    assert(m_debugger);
    if (m_runMode == RunMode::Normal)
    {
        togglePause(false);
    }
    bool breakpointHit;
    m_machine.update(RunMode::StepOver, breakpointHit);
    m_dissasemblyWindow.setCursor(getMachine().getZ80().PC());
    m_ui.clear();
    m_ui.render(std::bind(&Nx::drawDebugger, this, std::placeholders::_1));
}

void Nx::restart()
{
    m_machine.restart();
}

void Nx::keyPress(Key k, bool down)
{
    if (m_runMode != RunMode::Normal) return;
    m_keys[(int)k] = down;
}

void Nx::keysClear()
{
    std::fill(m_keys.begin(), m_keys.end(), false);
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
        bool result = m_machine.load(buffer, size, Machine::FileType::Sna);
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
    m_memoryDumpWindow.draw(draw);
    m_dissasemblyWindow.draw(draw);
    m_cpuStatusWindow.draw(draw);
}

void Nx::toggleDebugger()
{
    m_debugger = !m_debugger;
}

void Nx::uiKeyPress(UiKey k, bool down)
{
    m_memoryDumpWindow.keyPress(k, down);
    m_dissasemblyWindow.keyPress(k, down);
    m_cpuStatusWindow.keyPress(k, down);

    if (down)
    {
        using DK = UiKey;
        switch (k)
        {
        case DK::Tab:
            if (down)
            {
                if (m_memoryDumpWindow.isSelected())
                {
                    m_dissasemblyWindow.Select();
                }
                else
                {
                    m_memoryDumpWindow.Select();
                }
            }
            break;
                
        case DK::F5:
            togglePause(false);
            break;
            
        case DK::F6:
            stepOver();
            break;
            
        case DK::F7:
            stepIn();
            break;
            
        case DK::F8:
            break;
            
        default:
            break;
        }
    }
}

void Nx::togglePause(bool wasBreakpoint)
{
    m_runMode = (m_runMode != RunMode::Normal) ? RunMode::Normal : RunMode::Stopped;
    
    if (!m_debugger)
    {
        // If the debugger isn't running then we only show the debugger if we're pausing.
        m_debugger = (m_runMode == RunMode::Stopped);
    }

    // Because this method is usually called after a key press, which usually gets processed at the end of the frame,
    // the next instruction will be after an interrupt fired.  We step one more time to process the interrupt and
    // jump to the interrupt routine.  This requires that the debugger be activated.  Of course, we don't want this to happen
    // if a breakpoint occurs.
    if (!wasBreakpoint && m_debugger && m_runMode == RunMode::Stopped) stepIn();
    m_dissasemblyWindow.adjustBar();
    m_dissasemblyWindow.Select();

}

//----------------------------------------------------------------------------------------------------------------------
// Overlay
//----------------------------------------------------------------------------------------------------------------------

void Nx::drawOverlay(Ui::Draw& draw)
{
    if (m_runMode == RunMode::Stopped)
    {
        draw.printSquashedString(70, 60, "Stopped", draw.attr(Colour::Black, Colour::White, true));
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
