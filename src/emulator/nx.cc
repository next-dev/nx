//----------------------------------------------------------------------------------------------------------------------
// Nx class
//----------------------------------------------------------------------------------------------------------------------

#include <emulator/nx.h>
#include <emulator/nxfile.h>
#include <utils/tinyfiledialogs.h>
#include <utils/ui.h>

#include <algorithm>
#include <cassert>
#include <chrono>


#ifdef __APPLE__
#   include "ResourcePath.hpp"
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

    if (getEmulator().getZoom())
    {
        draw.printSquashedString(70, 58, "ZOOM!", draw.attr(Colour::Black, Colour::White, true));
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
        fill(m_speccyKeys.begin(), m_speccyKeys.end(), false);

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
            openFile();
            break;

        case K::S:
            saveFile();
            break;

        case K::T:
            getEmulator().showTapeBrowser();
            break;

        case K::A:
            getEmulator().showAssembler();
            break;

        case K::Space:
            if (getSpeccy().getTape())
            {
                getSpeccy().getTape()->toggle();
            }
            break;

        case K::Z:
            getEmulator().toggleZoom();


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
    bool mute = getSpeccy().getAudio().isMute();
    getSpeccy().getAudio().mute(true);

    const char* filters[] = { "*.nx", "*.sna", "*.z80", "*.tap" };

    const char* fileName = tinyfd_openFileDialog("Open file", 0,
        sizeof(filters)/sizeof(filters[0]), filters, "NX Files", 0);

    if (fileName)
    {
        if (!getEmulator().openFile(fileName))
        {
            MessageBoxA(0, "Unable to load!", "ERROR", MB_ICONERROR | MB_OK);
        }
    }

    getSpeccy().getAudio().mute(mute);

    getSpeccy().renderVideo();
    getEmulator().render();
}

void Emulator::saveFile()
{
    bool mute = getSpeccy().getAudio().isMute();
    getSpeccy().getAudio().mute(true);

    const char* filters[] = { "*.nx", "*.sna" };

    const char* fileName = tinyfd_saveFileDialog("Save snapshot", 0,
        sizeof(filters) / sizeof(filters[0]), filters, "Snapshot files");

    if (fileName)
    {
        if (!getEmulator().saveFile(fileName))
        {
            MessageBoxA(0, "Unable to save snapshot!", "ERROR", MB_ICONERROR | MB_OK);
        }
    }

    getSpeccy().getAudio().mute(mute);
}

bool Nx::openFile(string fileName)
{
    // Get extension
    fs::path path = fileName;

    if (path.has_extension())
    {
        string ext = path.extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), tolower);
        if (ext == ".sna")
        {
            return loadSnaSnapshot(fileName);
        }
        else if (ext == ".nx")
        {
            return loadNxSnapshot(fileName);
        }
        else if (ext == ".z80")
        {
            return loadZ80Snapshot(fileName);
        }
        else if (ext == ".tap")
        {
            return loadTape(fileName);
        }
    }
    
    return false;
}

bool Nx::saveFile(string fileName)
{
    // Get extension
    fs::path path = fileName;
    if (!path.has_extension())
    {
        path = (fileName += ".nx");
    }

    string ext = path.extension().string();
    transform(ext.begin(), ext.end(), ext.begin(), tolower);

    if (ext == ".sna")
    {
        return saveSnaSnapshot(fileName);
    }
    else if (ext == ".nx")
    {
        return saveNxSnapshot(fileName);
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

extern const u8 gRom48[16384];

Nx::Nx(int argc, char** argv)
    : m_machine(new Spectrum(std::bind(&Nx::frame, this)))   // #todo: Allow the debugger to switch Spectrums, via proxy
    , m_quit(false)
    , m_frameCounter(0)
    , m_zoom(false)
    , m_ui(*m_machine)

    //--- Emulator state ------------------------------------------------------------
    , m_emulator(*this)

    //--- Debugger state ------------------------------------------------------------
    , m_debugger(*this)
    , m_runMode(RunMode::Normal)

    //--- Assembler state -----------------------------------------------------------
    , m_assembler(*this)

    //--- Rendering -----------------------------------------------------------------
    , m_window(sf::VideoMode(kWindowWidth * kDefaultScale * 2, kWindowHeight * kDefaultScale * 2), "NX " NX_VERSION,
               sf::Style::Titlebar | sf::Style::Close)

    //--- Peripherals ---------------------------------------------------------------
    , m_kempstonJoystick(false)

    //--- Tape Browser --------------------------------------------------------------
    , m_tapeBrowser(*this)

    //--- Files ---------------------------------------------------------------------
    , m_tempPath()
{
    sf::FileInputStream f;
#ifdef __APPLE__
    m_tempPath = resourcePath();
#else
    m_tempPath = fs::path(argv[0]).parent_path();
#endif
    setScale(kDefaultScale);
    m_machine->getVideoSprite().setScale(float(kDefaultScale * 2), float(kDefaultScale * 2));
    m_ui.getSprite().setScale(float(kDefaultScale), float(kDefaultScale));

    //m_machine->load(0, loadFile(romFileName));
    m_machine->load(0, gRom48, 16384);
    m_machine->setRomWriteState(false);
    
    // Deal with the command line
    bool loadedFiles = false;
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
            loadedFiles = true;
        }
    }
    
    updateSettings();
    if (!loadedFiles)
    {
        loadNxSnapshot((m_tempPath / "cache.nx").string());
    }
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

void Nx::setScale(int scale)
{
    m_window.setSize({ unsigned(kWindowWidth * scale * 2), unsigned(kWindowHeight * scale * 2) });

    sf::Vector2i pos = m_window.getPosition();
    if (pos.x < 0 || pos.y < 0)
    {
        // Make sure the menu bar is on-screen
        m_window.setPosition({ 10, 10 });
    }
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
                if (!event.key.shift && event.key.control && !event.key.alt)
                {
                    // Possible global key
                    switch (event.key.code)
                    {
                    case sf::Keyboard::Key::Num1:
                        setScale(1);
                        break;

                    case sf::Keyboard::Key::Num2:
                        setScale(2);
                        break;

                    default:
                        Overlay::currentOverlay()->key(event.key.code, true, false, true, false);
                    }
                }
                else
                {
                    Overlay::currentOverlay()->key(event.key.code, true, event.key.shift, event.key.control, event.key.alt);
                }
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
        if (m_zoom || m_machine->getAudio().getSignal().isTriggered())
        {
            frame();
            render();
        }
    }

    // Shutdown
    saveNxSnapshot((m_tempPath / "cache.nx").string());
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
// Snapshot loading & saving
//----------------------------------------------------------------------------------------------------------------------

bool Nx::loadSnaSnapshot(string fileName)
{
    vector<u8> buffer = NxFile::loadFile(fileName);
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

bool Nx::loadZ80Snapshot(string fileName)
{
    vector<u8> buffer = NxFile::loadFile(fileName);
    u8* data = buffer.data();
    Z80& z80 = m_machine->getZ80();

    // Only support version 1.0 Z80 files now
    if (buffer.size() < 30) return false;
    int version = 1;
    if (WORD_OF(data, 6) == 0)
    {
        if (WORD_OF(data, 30) == 23) version = 2;
        else version = 3;
    }

    if (version > 1)
    {
        // Check to see if we're only 48K
        u8 hardware = BYTE_OF(data, 34);
        if (version == 2 && (hardware != 0 && hardware == 1)) return false;
        if (version == 3 && (hardware != 0 || hardware == 1 || hardware == 3)) return false;
    }

    z80.A() = BYTE_OF(data, 0);
    z80.F() = BYTE_OF(data, 1);
    z80.BC() = WORD_OF(data, 2);
    z80.HL() = WORD_OF(data, 4);
    z80.PC() = WORD_OF(data, 6);
    z80.SP() = WORD_OF(data, 8);
    z80.I() = BYTE_OF(data, 10);
    z80.R() = (BYTE_OF(data, 11) & 0x7f) | ((BYTE_OF(data, 12) & 0x01) << 7);
    u8 b12 = BYTE_OF(data, 12);
    if (b12 == 255) b12 = 1;
    m_machine->setBorderColour((b12 & 0x0e) >> 1);
    bool compressed = (b12 & 0x20) != 0;
    z80.DE() = WORD_OF(data, 13);
    z80.BC_() = WORD_OF(data, 15);
    z80.DE_() = WORD_OF(data, 17);
    z80.HL_() = WORD_OF(data, 19);
    u8 a_ = BYTE_OF(data, 21);
    u8 f_ = BYTE_OF(data, 22);
    z80.AF_() = (u16(a_) << 8) + u16(f_);
    z80.IY() = WORD_OF(data, 23);
    z80.IX() = WORD_OF(data, 25);
    z80.IFF1() = BYTE_OF(data, 27) ? 1 : 0;
    z80.IFF2() = BYTE_OF(data, 28) ? 1 : 0;
    z80.IM() = int(BYTE_OF(data, 29) & 0x03);

#define CHECK_BUFFER() do { if (size_t(mem - data) >= buffer.size()) { NX_BREAK(); return false; } } while(0)

    if (version == 1)
    {
        if (compressed)
        {
            u8* mem = data + 30;
            u16 a = 0x4000;
            while (1)
            {
                // Check we haven't run out of bytes.
                CHECK_BUFFER();
                u8 b = *mem++;
                if (b == 0x00)
                {
                    // Not enough room for 4 terminating bytes
                    if (size_t(mem + 3 - data) > buffer.size())
                    {
                        NX_BREAK();
                        return false;
                    }

                    if (mem[0] == 0xed && mem[1] == 0xed && mem[2] == 0x00)
                    {
                        // Terminator.
                        break;
                    }

                    m_machine->poke(a++, 0);
                }
                else if (b == 0xed)
                {
                    CHECK_BUFFER();
                    b = *mem++;
                    if (b != 0xed)
                    {
                        m_machine->poke(a++, 0xed);
                        m_machine->poke(a++, b);
                    }
                    else
                    {
                        // Two EDs - compression.
                        CHECK_BUFFER();
                        u8 count = *mem++;
                        CHECK_BUFFER();
                        b = *mem++;

                        for (u8 i = 0; i < count; ++i)
                        {
                            m_machine->poke(a++, b);
                        }
                    }
                }
                else
                {
                    m_machine->poke(a++, b);
                }
            }
        }
        else
        {
            if (buffer.size() != (0xc000 + 30)) return false;

            m_machine->load(0x4000, data + 30, 0xc000);
        }
    }
    else
    {
        // Version 2 & 3 files
        u8* mem = data + 32 + WORD_OF(data, 30);
        z80.PC() = WORD_OF(data, 32);
        if (version == 3)
        {
            m_machine->setTState(TState(WORD_OF(data, 55)) + (TState(BYTE_OF(data, 57)) << 16));
        }

        u16 pages[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0xc000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000 };
        for (int i = 0; i < 3; ++i)
        {
            u16 a = pages[BYTE_OF(mem,2)];
            u16 len = WORD_OF(mem, 0);
            mem += 3;
            bool compressed = (len != 0xffff);
            if (!compressed) len = 0x4000;

            int idx = 0;
            while (idx < len)
            {
                u8 b = mem[idx++];
                if (b == 0xed)
                {
                    b = mem[idx++];
                    if (b == 0xed)
                    {
                        u8 count = mem[idx++];
                        b = mem[idx++];
                        for (int ii = 0; ii < count; ++ii)
                        {
                            m_machine->poke(a++, b);
                        }
                    }
                    else
                    {
                        m_machine->poke(a++, 0xed);
                        m_machine->poke(a++, b);
                    }
                }
                else
                {
                    m_machine->poke(a++, b);
                }
            }
            mem += len;
        }
    }

    return true;
}

bool Nx::saveSnaSnapshot(string fileName)
{
    vector<u8> data;
    Z80& z80 = m_machine->getZ80();

    TState t = 0;
    z80.push(z80.PC(), t);

    NxFile::write8(data, z80.I());
    NxFile::write16(data, z80.HL_());
    NxFile::write16(data, z80.DE_());
    NxFile::write16(data, z80.BC_());
    NxFile::write16(data, z80.AF_());
    NxFile::write16(data, z80.HL());
    NxFile::write16(data, z80.DE());
    NxFile::write16(data, z80.BC());
    NxFile::write16(data, z80.IY());
    NxFile::write16(data, z80.IX());
    NxFile::write8(data, (z80.IFF1() ? 0x01 : 0) | (z80.IFF2() ? 0x04 : 0));
    NxFile::write8(data, z80.R());
    NxFile::write16(data, z80.AF());
    NxFile::write16(data, z80.SP());
    NxFile::write8(data, (u8)z80.IM());
    NxFile::write8(data, m_machine->getBorderColour());
    for (u16 a = 0x4000; a; ++a)
    {
        data.emplace_back(m_machine->peek(a));
    }

    z80.pop(t);

    return NxFile::saveFile(fileName, data);
}

bool Nx::loadNxSnapshot(string fileName)
{
    NxFile f;

    if (f.load(fileName) &&
        f.checkSection('SN48', 36) &&
        f.checkSection('RM48', 49152))
    {
        const BlockSection& sn48 = f['SN48'];
        const BlockSection& rm48 = f['RM48'];
        Z80& z80 = m_machine->getZ80();

        z80.AF() = sn48.peek16(0);
        z80.BC() = sn48.peek16(2);
        z80.DE() = sn48.peek16(4);
        z80.HL() = sn48.peek16(6);
        z80.AF_() = sn48.peek16(8);
        z80.BC_() = sn48.peek16(10);
        z80.DE_() = sn48.peek16(12);
        z80.HL_() = sn48.peek16(14);
        z80.IX() = sn48.peek16(16);
        z80.IY() = sn48.peek16(18);
        z80.SP() = sn48.peek16(20);
        z80.PC() = sn48.peek16(22);
        z80.IR() = sn48.peek16(24);
        z80.MP() = sn48.peek16(26);
        z80.IM() = (int)sn48.peek8(28);
        z80.IFF1() = sn48.peek8(29) != 0;
        z80.IFF2() = sn48.peek8(30) != 0;
        m_machine->setBorderColour(sn48.peek8(31));
        m_machine->setTState((TState)sn48.peek32(32));

        m_machine->load(0x4000, rm48.data());

        return true;
    }

    return false;
}

bool Nx::saveNxSnapshot(string fileName)
{
    NxFile f;
    Z80& z80 = m_machine->getZ80();

    BlockSection sn48('SN48');
    sn48.poke16(z80.AF());
    sn48.poke16(z80.BC());
    sn48.poke16(z80.DE());
    sn48.poke16(z80.HL());
    sn48.poke16(z80.AF_());
    sn48.poke16(z80.BC_());
    sn48.poke16(z80.DE_());
    sn48.poke16(z80.HL_());
    sn48.poke16(z80.IX());
    sn48.poke16(z80.IY());
    sn48.poke16(z80.SP());
    sn48.poke16(z80.PC());
    sn48.poke16(z80.IR());
    sn48.poke16(z80.MP());
    sn48.poke8((u8)z80.IM());
    sn48.poke8(z80.IFF1() ? 1 : 0);
    sn48.poke8(z80.IFF2() ? 1 : 0);
    sn48.poke8(m_machine->getBorderColour());
    sn48.poke32((u32)m_machine->getTState());
    f.addSection(sn48, 36);

    BlockSection rm48('RM48');
    for (u16 a = 0x4000; a != 0x0000; ++a)
    {
        rm48.poke8(m_machine->peek(a));
    }
    f.addSection(rm48, 49152);

    return f.save(fileName);
}

//----------------------------------------------------------------------------------------------------------------------
// Tape loading
//----------------------------------------------------------------------------------------------------------------------

bool Nx::loadTape(string fileName)
{
    vector<u8> file = NxFile::loadFile(fileName);
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

void Nx::stepOut()
{
    if (m_runMode == RunMode::Normal) togglePause(false);
    else
    {
        u16 sp = getSpeccy().getZ80().SP();
        TState t = 0;
        u16 address = m_machine->peek16(sp, t);
        m_machine->addTemporaryBreakpoint(address);
        m_runMode = RunMode::Normal;
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
// Zooming
//----------------------------------------------------------------------------------------------------------------------

void Nx::toggleZoom()
{
    m_zoom = !m_zoom;
    getSpeccy().getAudio().mute(m_zoom);
}

//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

void Nx::showAssembler()
{
    m_assembler.select();
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------














































