//----------------------------------------------------------------------------------------------------------------------
// Nx class
//----------------------------------------------------------------------------------------------------------------------

#include "nx.h"
#include "ui.h"

#include <algorithm>
#include <cassert>
#include <chrono>


#ifdef __APPLE__
#   include "ResourcePath.hpp"
#endif

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>
#endif


// Scaling constants for the Speccy pixel to PC pixel ratio.
static const int kScale = 4;
static const int kUiScale = 2;

//----------------------------------------------------------------------------------------------------------------------
// File open dialog
//----------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32

typedef struct
{
    const char*   title;
    const char*   path;
    const char*   filterName;
    const char*   filter;
}
WindowFileOpenConfig;

const char* windowFileOpen(WindowFileOpenConfig* config);
const char* windowFileSaveAs(WindowFileOpenConfig* config);

#include <commdlg.h>
#include <string.h>

static int win32NextFilter(char* dst, const char** p)
{
    int len;

    *p += strspn(*p, "|");
    if (**p == '\0') {
        return 0;
    }

    len = (int)strcspn(*p, "|");
    memcpy(dst, *p, len);
    dst[len] = '\0';
    *p += len;

    return 1;
}

static const char* win32MakeFilterString(WindowFileOpenConfig* config)
{
    static char buf[1024];
    int n = 0;

    buf[0] = 0;

    if (config->filter)
    {
        const char* p;
        char b[32];
        const char* name = config->filterName ? config->filterName : config->filter;

        n += sprintf(buf + n, "%s", name) + 1;

        p = config->filter;
        while (win32NextFilter(b, &p))
        {
            n += sprintf(buf + n, "%s;", b);
        }

        buf[++n] = 0;
    }

    n += sprintf(buf + n, "All Files") + 1;
    n += sprintf(buf + n, "*.*");
    buf[++n] = 0;

    return buf;
}

static void win32InitOpenFileName(WindowFileOpenConfig* config, OPENFILENAMEA* ofn)
{
    static char fileName[2048];

    fileName[0] = 0;
    memset(ofn, 0, sizeof(*ofn));
    ofn->lStructSize = sizeof(*ofn);
    ofn->lpstrFilter = win32MakeFilterString(config);
    ofn->nFilterIndex = 1;
    ofn->lpstrFile = fileName;
    ofn->Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn->nMaxFile = sizeof(fileName) - 1;
    ofn->lpstrInitialDir = config->path;
    ofn->lpstrTitle = config->title;
    ofn->lpstrTitle = config->title;
}

const char* windowFileOpen(WindowFileOpenConfig* config)
{
    OPENFILENAMEA ofn;
    win32InitOpenFileName(config, &ofn);
    return GetOpenFileNameA(&ofn) ? ofn.lpstrFile : 0;
}

const char* windowFileSaveAs(WindowFileOpenConfig* config)
{
    OPENFILENAMEA ofn;
    win32InitOpenFileName(config, &ofn);
    return GetSaveFileNameA(&ofn) ? ofn.lpstrFile : 0;
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// Emulator overlay
//----------------------------------------------------------------------------------------------------------------------

Emulator::Emulator(Nx& nx)
    : Overlay(nx)
    , m_speccyKeys((int)Key::COUNT)
    , m_keyRows(8)
    , m_counter(0)
{

}

void Emulator::render(Draw& draw)
{
    if (getEmulator().getRunMode() == RunMode::Stopped)
    {
        draw.printSquashedString(70, 60, "Stopped", draw.attr(Colour::Black, Colour::White, true));
    }

    u8 colour = draw.attr(Colour::Red, Colour::White, true);

    if (m_counter > 0)
    {
        draw.printSquashedString(1, 62,
            draw.format("Kempston Joystick: %s", getEmulator().usesKempstonJoystick() ? "Enabled" : "Disabled"),
            colour);
        --m_counter;
    }
}

void Emulator::showStatus()
{
    m_counter = 100;
}

void Emulator::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    Key key1 = Key::COUNT;
    Key key2 = Key::COUNT;

    using K = sf::Keyboard;

    if (down && ctrl && !shift && !alt)
    {
        switch (key)
        {
        case K::K:
            getEmulator().setSetting("kempston", getEmulator().getSetting("kempston") == "yes" ? "no" : "yes");
            getEmulator().updateSettings();
            showStatus();
            break;

        case K::R:
            getSpeccy().reset(false);
            break;

        case K::O:
            fill(m_speccyKeys.begin(), m_speccyKeys.end(), false);
            openFile();
            break;

        case K::T:
            getEmulator().showTapeBrowser();
            break;

        case K::Space:
            if (getSpeccy().getTape())
            {
                getSpeccy().getTape()->toggle();
            }
            break;

        default:
            break;
        }
    }
    else
    {
        switch (key)
        {
            //
            // Numbers
            //
        case K::Num1:       key1 = Key::_1;         break;
        case K::Num2:       key1 = Key::_2;         break;
        case K::Num3:       key1 = Key::_3;         break;
        case K::Num4:       key1 = Key::_4;         break;
        case K::Num5:       key1 = Key::_5;         break;
        case K::Num6:       key1 = Key::_6;         break;
        case K::Num7:       key1 = Key::_7;         break;
        case K::Num8:       key1 = Key::_8;         break;
        case K::Num9:       key1 = Key::_9;         break;
        case K::Num0:       key1 = Key::_0;         break;

            //
            // Letters
            //
        case K::A:          key1 = Key::A;          break;
        case K::B:          key1 = Key::B;          break;
        case K::C:          key1 = Key::C;          break;
        case K::D:          key1 = Key::D;          break;
        case K::E:          key1 = Key::E;          break;
        case K::F:          key1 = Key::F;          break;
        case K::G:          key1 = Key::G;          break;
        case K::H:          key1 = Key::H;          break;
        case K::I:          key1 = Key::I;          break;
        case K::J:          key1 = Key::J;          break;
        case K::K:          key1 = Key::K;          break;
        case K::L:          key1 = Key::L;          break;
        case K::M:          key1 = Key::M;          break;
        case K::N:          key1 = Key::N;          break;
        case K::O:          key1 = Key::O;          break;
        case K::P:          key1 = Key::P;          break;
        case K::Q:          key1 = Key::Q;          break;
        case K::R:          key1 = Key::R;          break;
        case K::S:          key1 = Key::S;          break;
        case K::T:          key1 = Key::T;          break;
        case K::U:          key1 = Key::U;          break;
        case K::V:          key1 = Key::V;          break;
        case K::W:          key1 = Key::W;          break;
        case K::X:          key1 = Key::X;          break;
        case K::Y:          key1 = Key::Y;          break;
        case K::Z:          key1 = Key::Z;          break;

            //
            // Other keys on the Speccy
            //
        case K::LShift:     key1 = Key::Shift;      break;
        case K::RShift:     key1 = Key::SymShift;   break;
        case K::Return:     key1 = Key::Enter;      break;
        case K::Space:      key1 = Key::Space;      break;

            //
            // Map PC keys to various keys on the Speccy
            //
        case K::BackSpace:  key1 = Key::Shift;      key2 = Key::_0;     break;
        case K::Escape:     key1 = Key::Shift;      key2 = Key::Space;  break;

        case K::SemiColon:
            key1 = Key::SymShift;
            key2 = shift ? Key::Z : Key::O;
            break;

        case K::Comma:
            key1 = Key::SymShift;
            key2 = shift ? Key::R : Key::N;
            break;

        case K::Period:
            key1 = Key::SymShift;
            key2 = shift ? Key::T : Key::M;
            break;

        case K::Quote:
            key1 = Key::SymShift;
            key2 = shift ? Key::P : Key::_7;
            break;

        case K::Slash:
            key1 = Key::SymShift;
            key2 = shift ? Key::C : Key::V;
            break;

        case K::Dash:
            key1 = Key::SymShift;
            key2 = shift ? Key::_0 : Key::J;
            break;

        case K::Equal:
            key1 = Key::SymShift;
            key2 = shift ? Key::K : Key::L;
            break;

        case K::Left:
            if (getEmulator().usesKempstonJoystick())
            {
                joystickKey(Joystick::Left, down);
            }
            else
            {
                key1 = Key::Shift;
                key2 = Key::_5;
            }
            break;

        case K::Down:
            if (getEmulator().usesKempstonJoystick())
            {
                joystickKey(Joystick::Down, down);
            }
            else
            {
                key1 = Key::Shift;
                key2 = Key::_6;
            }
            break;

        case K::Up:
            if (getEmulator().usesKempstonJoystick())
            {
                joystickKey(Joystick::Up, down);
            }
            else
            {
                key1 = Key::Shift;
                key2 = Key::_7;
            }
            break;

        case K::Right:
            if (getEmulator().usesKempstonJoystick())
            {
                joystickKey(Joystick::Right, down);
            }
            else
            {
                key1 = Key::Shift;
                key2 = Key::_8;
            }
            break;

        case K::Tab:
            if (getEmulator().usesKempstonJoystick())
            {
                joystickKey(Joystick::Fire, down);
            }
            else
            {
                key1 = Key::Shift;
                key2 = Key::SymShift;
            }
            break;

        case K::Tilde:
            if (down) getEmulator().toggleDebugger();
            break;

        case K::F5:
            if (down) getEmulator().togglePause(false);
            break;

        default:
            // If releasing a non-speccy key, clear all key map.
            fill(m_speccyKeys.begin(), m_speccyKeys.end(), false);
        }
    }

    if (key1 != Key::COUNT)
    {
        m_speccyKeys[(int)key1] = down;
    }
    if (key2 != Key::COUNT)
    {
        m_speccyKeys[(int)key2] = down;
    }

    // Fix for Windows crappy keyboard support.  It's not perfect but better than not dealing with it.
#ifdef _WIN32
    if ((key == K::LShift || key == K::RShift) && !down)
    {
        m_speccyKeys[(int)Key::Shift] = false;
        m_speccyKeys[(int)Key::SymShift] = false;
    }
#endif

    calculateKeys();
}

void Emulator::calculateKeys()
{
    for (int i = 0; i < 8; ++i)
    {
        u8 keys = 0;
        u8 key = 1;
        for (int j = 0; j < 5; ++j)
        {
            if (m_speccyKeys[i * 5 + j])
            {
                keys += key;
            }
            key <<= 1;
        }
        m_keyRows[i] = keys;
    }

    getSpeccy().setKeyboardState(m_keyRows);
}

void Emulator::text(char ch)
{
    
}

void Emulator::joystickKey(Joystick key, bool down)
{
    u8 bit = 0;

    switch (key)
    {
    case Joystick::Right:    bit = 0x01;     break;
    case Joystick::Left:     bit = 0x02;     break;
    case Joystick::Down:     bit = 0x04;     break;
    case Joystick::Up:       bit = 0x08;     break;
    case Joystick::Fire:     bit = 0x10;     break;
    }


    if (down)
    {
        getSpeccy().setKempstonState(getSpeccy().getKempstonState() | bit);
    }
    else
    {
        getSpeccy().setKempstonState(getSpeccy().getKempstonState() & ~bit);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// File opening
//----------------------------------------------------------------------------------------------------------------------

void Emulator::openFile()
{
#ifdef _WIN32
    // Last path opened
    static char path[2048] = { 0 };

    // Open file
    WindowFileOpenConfig cfg = {
        "Open file",
        path,
        "NX files",
        "*.sna;*.tap"
    };
    const char* fileName = windowFileOpen(&cfg);

    if (fileName)
    {
        // Store the path for next time
        const char* end = strrchr(fileName, '\\');
        strncpy(path, (const char *)fileName, (size_t)(path - end));

        if (!getEmulator().openFile(fileName))
        {
            MessageBoxA(0, "Unable to load!", "ERROR", MB_ICONERROR | MB_OK);
        }
    }
#endif
}

bool Nx::openFile(string fileName)
{
    // Get extension
    auto extIt = fileName.find_last_of('.');
    if (extIt != string::npos)
    {
        string ext = fileName.substr(extIt + 1);
        
        if (ext == "sna")
        {
            return loadSnapshot(fileName);
        }
        else if (ext == "tap")
        {
            return loadTape(fileName);
        }
    }
    
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Nx::Nx(int argc, char** argv)
    : m_machine(new Spectrum(std::bind(&Nx::frame, this)))   // #todo: Allow the debugger to switch Spectrums, via proxy
    , m_quit(false)
    , m_frameCounter(0)
    , m_ui(*m_machine)

    //--- Emulator state ------------------------------------------------------------
    , m_emulator(*this)

    //--- Debugger state ------------------------------------------------------------
    , m_debugger(*this)
    , m_runMode(RunMode::Normal)

    //--- Rendering -----------------------------------------------------------------
    , m_window(sf::VideoMode(kWindowWidth * kScale, kWindowHeight * kScale), "NX " NX_VERSION,
        sf::Style::Titlebar | sf::Style::Close)

    //--- Peripherals ---------------------------------------------------------------
    , m_kempstonJoystick(false)

    //--- Tape Browser --------------------------------------------------------------
    , m_tapeBrowser(*this)
{
    sf::FileInputStream f;
#ifdef __APPLE__
    string romFileName = resourcePath() + "48.rom";
#else
    string romFileName = "48.rom";
#endif
    m_machine->load(0, loadFile(romFileName));
    m_machine->setRomWriteState(false);
    
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
            openFile(arg);
        }
    }
    
    updateSettings();
    m_emulator.select();
}

//----------------------------------------------------------------------------------------------------------------------
// Destruction
//----------------------------------------------------------------------------------------------------------------------

Nx::~Nx()
{
    delete m_machine;
}

//----------------------------------------------------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------------------------------------------------

void Nx::render()
{
    m_window.clear();
    m_window.draw(m_machine->getVideoSprite());
    m_ui.render((m_frameCounter++ & 16) != 0);
    m_window.draw(m_ui.getSprite());
    m_window.display();
}

//----------------------------------------------------------------------------------------------------------------------
// Running
//----------------------------------------------------------------------------------------------------------------------

void Nx::run()
{
    while (m_window.isOpen())
    {
        sf::Event event;

        sf::Clock clk;

        //
        // Process the OS events
        //
        while (m_window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                m_quit = true;
                m_window.close();
                break;

            case sf::Event::KeyPressed:
                // Forward the key controls to the right mode handler
                Overlay::currentOverlay()->key(event.key.code, true, event.key.shift, event.key.control, event.key.alt);
                break;

            case sf::Event::KeyReleased:
                // Forward the key controls to the right mode handler
                Overlay::currentOverlay()->key(event.key.code, false, event.key.shift, event.key.control, event.key.alt);
                break;
                    
            case sf::Event::TextEntered:
                Overlay::currentOverlay()->text((char)event.text.unicode);
                break;

            default:
                break;
            }
        }

        //
        // Generate a frame
        //
        if (m_machine->getAudio().getSignal().isTriggered())
        {
            frame();
            render();
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Frame generation
//----------------------------------------------------------------------------------------------------------------------

void Nx::frame()
{
    if (m_quit) return;
    bool breakpointHit = false;
    m_machine->update(m_runMode, breakpointHit);
    if (breakpointHit)
    {
        m_debugger.getDisassemblyWindow().setCursor(m_machine->getZ80().PC());
        togglePause(true);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// File loading
//----------------------------------------------------------------------------------------------------------------------

vector<u8> Nx::loadFile(string fileName)
{
    vector<u8> buffer;
    sf::FileInputStream f;
    
    if (f.open(fileName))
    {
        i64 size = f.getSize();
        buffer.resize(size);
        f.read(buffer.data(), size);
    }
    
    return buffer;
}

//----------------------------------------------------------------------------------------------------------------------
// Snapshot loading
//----------------------------------------------------------------------------------------------------------------------

bool Nx::loadSnapshot(string fileName)
{
    vector<u8> buffer = loadFile(fileName);
    u8* data = buffer.data();
    i64 size = (i64)buffer.size();
    Z80& z80 = m_machine->getZ80();
    
    if (size != 49179) return false;
    
    z80.I() = BYTE_OF(data, 0);
    z80.HL_() = WORD_OF(data, 1);
    z80.DE_() = WORD_OF(data, 3);
    z80.BC_() = WORD_OF(data, 5);
    z80.AF_() = WORD_OF(data, 7);
    z80.HL() = WORD_OF(data, 9);
    z80.DE() = WORD_OF(data, 11);
    z80.BC() = WORD_OF(data, 13);
    z80.IY() = WORD_OF(data, 15);
    z80.IX() = WORD_OF(data, 17);
    z80.IFF1() = (BYTE_OF(data, 19) & 0x01) != 0;
    z80.IFF2() = (BYTE_OF(data, 19) & 0x04) != 0;
    z80.R() = BYTE_OF(data, 20);
    z80.AF() = WORD_OF(data, 21);
    z80.SP() = WORD_OF(data, 23);
    z80.IM() = BYTE_OF(data, 25);
    m_machine->setBorderColour(BYTE_OF(data, 26));
    m_machine->load(0x4000, data + 27, 0xc000);
    
    TState t = 0;
    z80.PC() = z80.pop(t);
    z80.IFF1() = z80.IFF2();
    m_machine->resetTState();
    
    return true;

}

//----------------------------------------------------------------------------------------------------------------------
// Tape loading
//----------------------------------------------------------------------------------------------------------------------

bool Nx::loadTape(string fileName)
{
    vector<u8> file = loadFile(fileName);
    if (file.size())
    {
        Tape* tape = m_tapeBrowser.loadTape(file);
        getSpeccy().setTape(tape);
        return true;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Settings
//----------------------------------------------------------------------------------------------------------------------

void Nx::setSetting(string key, string value)
{
    m_settings[key] = value;
}

string Nx::getSetting(string key, string defaultSetting)
{
    auto it = m_settings.find(key);
    return it == m_settings.end() ? defaultSetting : it->second;
}

void Nx::updateSettings()
{
    m_kempstonJoystick = getSetting("kempston") == "yes";
}

//----------------------------------------------------------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------------------------------------------------------

void Nx::toggleDebugger()
{
    m_debugger.toggle(m_emulator);
}

void Nx::togglePause(bool breakpointHit)
{
    m_runMode = (m_runMode != RunMode::Normal) ? RunMode::Normal : RunMode::Stopped;
    m_machine->getAudio().mute(m_runMode == RunMode::Stopped);

    if (!isDebugging())
    {
        // If the debugger isn't running then we only show the debugger if we're pausing.
        m_debugger.selectIf(m_runMode == RunMode::Stopped, m_emulator);
    }

    // Because this method is usually called after a key press, which usually gets processed at the end of the frame,
    // the next instruction will be after an interrupt fired.  We step one more time to process the interrupt and
    // jump to the interrupt routine.  This requires that the debugger be activated.  Of course, we don't want this to happen
    // if a breakpoint occurs.
    if (!breakpointHit && isDebugging() && m_runMode == RunMode::Stopped) stepIn();
    m_debugger.getDisassemblyWindow().adjustBar();
    m_debugger.getDisassemblyWindow().Select();
}

void Nx::stepIn()
{
    assert(isDebugging());
    if (m_runMode == RunMode::Normal) togglePause(false);

    bool breakpointHit;
    m_machine->update(RunMode::StepIn, breakpointHit);
    m_debugger.getDisassemblyWindow().setCursor(m_machine->getZ80().PC());
}

void Nx::stepOver()
{
    u16 pc = getSpeccy().getZ80().PC();
    if (isCallInstructionAt(pc))
    {
        assert(isDebugging());
        if (m_runMode == RunMode::Normal) togglePause(false);

        // #todo: use assembler and static analysis to better support where to place the BP (e.g. trailing params).
        pc = nextInstructionAt(pc);
        m_machine->addTemporaryBreakpoint(pc);
        m_runMode = RunMode::Normal;
    }
    else
    {
        stepIn();
    }
}

u16 Nx::nextInstructionAt(u16 address)
{
    return m_debugger.getDisassemblyWindow().disassemble(address);
}

bool Nx::isCallInstructionAt(u16 address)
{
    u8 opCode = getSpeccy().peek(address);

    switch (opCode)
    {
    case 0xc4:  // call nz,nnnn
    case 0xcc:  // call z,nnnn
    case 0xcd:  // call nnnn
    case 0xd4:  // call nc,nnnn
    case 0xdc:  // call c,nnnn
    case 0xe4:  // call po,nnnn
    case 0xec:  // call pe,nnnn
    case 0xf4:  // call p,nnnn
    case 0xfc:  // call m,nnnn

    case 0xc7:  // rst 0
    case 0xcf:  // rst 8
    case 0xd7:  // rst 16
    case 0xdf:  // rst 24
    case 0xe7:  // rst 32
    case 0xef:  // rst 40
    case 0xf7:  // rst 48
    case 0xff:  // rst 56
        return true;

    default:
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Tape browser
//----------------------------------------------------------------------------------------------------------------------

void Nx::showTapeBrowser()
{
    m_tapeBrowser.select();
}

void Nx::hideAll()
{
    m_emulator.select();
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------














































