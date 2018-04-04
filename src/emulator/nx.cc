//----------------------------------------------------------------------------------------------------------------------
// Nx class
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <editor/editor.h>
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
// Model selection
//----------------------------------------------------------------------------------------------------------------------

ModelWindow::ModelWindow(Nx& nx)
    : Window(nx, 1, 1, 20, 2 + (int)Model::COUNT, "Select model", Colour::Black, Colour::White, true)
    , m_models({ Model::ZX48, Model::ZX128, Model::ZXPlus2 })
    , m_selectedModel(-1)
{

}

void ModelWindow::onDraw(Draw& draw)
{
    static const char* modelNames[(int)Model::COUNT] = {
        "ZX Spectrum 48K",
        "ZX Spectrum 128K",
        "ZX Spectrum +2",
    };

    assert(m_models.size() == (int)Model::COUNT);

    for (int i = 0; i < (int)Model::COUNT; ++i)
    {
        draw.printSquashedString(m_x + 2, m_y + 1+i, string(modelNames[(int)m_models[i]]), draw.attr(Colour::Black, Colour::White, true));
        if (i == 0) draw.printChar(m_x + 1, m_y + 1, '*', draw.attr(Colour::Black, Colour::White, true));
        if (i == m_selectedModel)
        {
            draw.attrRect(m_x + 1, m_y + 1 + i, m_width - 2, 1, draw.attr(Colour::White, Colour::Red, true));
        }
    }
}

void ModelWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;

    if (down && ctrl && !alt && !shift && key == K::Tab)
    {
        if (m_selectedModel == -1)
        {
            // Pressing CTRL-TAB for the first time, select the 2nd item
            m_selectedModel = 1;
        }
        else
        {
            // Press TAB again while pressing CTRL (as releasing CTRL would make this logic unreachable)
            ++m_selectedModel;
        }
        if (m_selectedModel >= (int)Model::COUNT) m_selectedModel = 0;
    }

    if ((m_selectedModel >= 0) && !down && !ctrl && !shift && !alt)
    {
        // CTRL-TAB has been released while selecting model.
        if (m_selectedModel != 0)
        {
            Model newModel = m_models[m_selectedModel];
            m_nx.switchModel(newModel);
        }
        m_selectedModel = -1;
    }
}

void ModelWindow::switchModel(Model model)
{
    auto it = find(m_models.begin(), m_models.end(), model);
    m_models.erase(it);
    m_models.insert(m_models.begin(), model);
}

//----------------------------------------------------------------------------------------------------------------------
// Emulator overlay
//----------------------------------------------------------------------------------------------------------------------

Emulator::Emulator(Nx& nx)
    : Overlay(nx)
    , m_speccyKeys((int)Key::COUNT)
    , m_keyRows(8)
    , m_counter(0)
    , m_modelWindow(nx)
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

    if (m_modelWindow.visible()) m_modelWindow.draw(draw);
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

    if (m_modelWindow.visible())
    {
        m_modelWindow.keyPress(key, down, shift, ctrl, alt);
    }
    else if (down && ctrl && !shift && !alt)
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
            getSpeccy().reset(getSpeccy().getModel());
            getEmulator().getDebugger().getDisassemblyWindow().setLabels(Labels{});
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
            getEmulator().showEditor();
            break;

        case K::D:
            getEmulator().showDisassembler();
            break;

        case K::Space:
            if (getSpeccy().getTape())
            {
                getSpeccy().getTape()->toggle();
            }
            break;

        case K::Z:
            getEmulator().toggleZoom();

        case K::Tab:
            // Switch model - let the model window handle it
            m_modelWindow.keyPress(key, down, shift, ctrl, alt);
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
        return saveNxSnapshot(fileName, false);
    }

    return false;
}

void Emulator::switchModel(Model model)
{
    m_modelWindow.switchModel(model);
}

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

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
    , m_editorOverlay(*this)
    , m_assemblerOverlay(*this)
    , m_assembler(m_assemblerOverlay.getWindow(), *m_machine)

    //--- Disassembler state --------------------------------------------------------
    , m_disassemblerOverlay(*this)

    //--- Rendering -----------------------------------------------------------------
    , m_window(sf::VideoMode(kWindowWidth * (kDefaultScale + 1), kWindowHeight * (kDefaultScale + 1)), "NX " NX_VERSION,
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
    m_machine->getVideoSprite().setScale(float(kDefaultScale + 1), float(kDefaultScale + 1));
    m_ui.getSprite().setScale(float(kDefaultScale + 1) / 2, float(kDefaultScale + 1) / 2);

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
    unsigned windowWidth = unsigned(kWindowWidth * (scale + 1));
    unsigned windowheight = unsigned(kWindowHeight * (scale + 1));

    m_window.setSize({ windowWidth, windowheight });

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
                if (m_editorOverlay.getWindow().needToSave())
                {
                    int result = tinyfd_messageBox(
                        "Unsaved files detected",
                        "There are some unsaved changes in some edited files.  Do you wish to save "
                        "these files before continuing?",
                        "yesnocancel", "question", 0);
                    bool skipThisFile = false;
                    switch (result)
                    {
                    case 0:     // Cancel - stop everything!
                        m_quit = false;
                        break;

                    case 1:     // Yes - trigger save of unnamed/unsaved files
                        m_editorOverlay.getWindow().saveAll();
                        // continue

                    case 2:     // No - do not save
                        m_quit = true;
                        break;
                    }
                }
                else
                {
                    m_quit = true;
                }

                if (m_quit)
                {
                    m_window.close();
                }
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

                    case sf::Keyboard::Key::Num3:
                        setScale(2);
                        break;

                    case sf::Keyboard::Key::Num2:
                        setScale(3);
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
    saveNxSnapshot((m_tempPath / "cache.nx").string(), true);
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

    if (f.load(fileName))
    {
        // Find which model we should be in.  No MODL section, then assume 48K
        Model m = Model::ZX48;
        if (f.checkSection('MODL', 1))
        {
            const BlockSection& modl = f['MODL'];
            int model = modl.peek8(0);
            if (model < 0 || model >= (int)Model::COUNT) return false;
            m = (Model)model;
        }
        switchModel(m);

        switch (m)
        {
        case Model::ZXPlus2:
        case Model::ZX128:
            if (f.checkSection('S128', 1))
            {
                const BlockSection& s128 = f['S128'];
                TState t = 0;
                m_machine->out(0x7ffd, s128.peek8(0), t);
            }
            else
            {
                return false;
            }
            // Continue to 48K data

        case Model::ZX48:
            if (f.checkSection('SN48', 36))
            {
                const BlockSection& sn48 = f['SN48'];
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
            }
            else
            {
                return false;
            }

            if (m == Model::ZX48 && f.checkSection('RM48', 49152))
            {
                const BlockSection& rm48 = f['RM48'];
                m_machine->load(0x4000, rm48.data());
            }
            else if ((m == Model::ZX128 || m == Model::ZX128 || m == Model::ZXPlus2) &&
                f.checkSection('R128', 131072))
            {
                const BlockSection& r128 = f['R128'];
                for (int i = 0; i < 8; ++i)
                {
                    m_machine->page(3, i);
                    m_machine->load(0xc000, r128.data().data() + (i * KB(16)), KB(16));
                }
            }
            else
            {
                return false;
            }
            break;
        }

        if (f.checkSection('EMUL', 0))
        {
            const BlockSection& emul = f['EMUL'];
            int numFiles = 0;
            int numLabels = 0;

            int dataIndex;

            u16 version = emul.peek16(0);

            if (version >= 0)
            {
                numFiles = int(emul.peek16(2));
                dataIndex = 4;
            }
            if (version >= 1)
            {
                numLabels = int(emul.peek16(4));
                dataIndex = 6;
            }

            // Dealing with version 0 data
            for (int i = 0; i < numFiles; ++i)
            {
                string fn = emul.peekString(dataIndex);
                dataIndex += int(fn.size()) + 1;

                // Check to see if we don't already have it loaded
                int numEditors = m_editorOverlay.getWindow().getNumEditors();
                bool addEditor = true;
                for (int ed = 0; ed < numEditors; ++ed)
                {
                    if (m_editorOverlay.getWindow().getEditor(ed).getFileName() == fn)
                    {
                        addEditor = false;
                        break;
                    }
                }

                // Attempt to load it.
                m_editorOverlay.getWindow().openFile(fn);
            }

            // Dealing with version 1 data
            Labels labels;
            for (int i = 0; i < numLabels; ++i)
            {
                MemoryMap::Address addr = emul.peek32(dataIndex);
                dataIndex += 4;
                string label = emul.peekString(dataIndex);
                dataIndex += int(label.size()) + 1;
                labels.emplace_back(make_pair(label, addr));
            }
            m_debugger.getDisassemblyWindow().setLabels(labels);

        }

        return true;
    } // if (f.load(...

    return false;
}

bool Nx::saveNxSnapshot(string fileName, bool saveEmulatorSettings)
{
    NxFile f;
    Z80& z80 = m_machine->getZ80();
    Model model = m_machine->getModel();

    // Write out the 'MODL' section
    BlockSection modl('MODL');
    modl.poke8((u8)model);
    f.addSection(modl, 1);

    // Write out the 'SN48' section
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

    // Write out the 'S128' section if 128K
    if (model == Model::ZX128 ||
        model == Model::ZXPlus2)
    {
        BlockSection s128('S128');

        // Build the last value in $7ffd
        u8 io = 0;
        assert(m_machine->getPage(1) == 5);
        assert(m_machine->getPage(2) == 2);
        assert(m_machine->getPage(3) >= 0 && m_machine->getPage(3) < 8);
        io = u8(m_machine->getPage(3));
        if (m_machine->isShadowScreen()) io |= 0x08;
        if (m_machine->getPage(0) == 9) io |= 0x10;
        if (m_machine->isPagingDisabled()) io |= 0x20;

        s128.poke8(io);
        f.addSection(s128, 1);

        // Save out the memory
        BlockSection r128('R128');
        TState t = 0;
        int oldSlot3 = m_machine->getPage(3);
        for (int i = 0; i < 8; ++i)
        {
            m_machine->page(3, i);
            for (u16 byte = 0xc000; byte != 0x0000; ++byte)
            {
                r128.poke8(m_machine->peek(byte, t));
            }
        }
        m_machine->page(3, oldSlot3);
        f.addSection(r128, 131072);
    }
    else if (model == Model::ZX48)
    {
        BlockSection rm48('RM48');
        for (u16 a = 0x4000; a != 0x0000; ++a)
        {
            rm48.poke8(m_machine->peek(a));
        }
        f.addSection(rm48, 49152);
    }

    // Write out the 'EMUL' section
    if (saveEmulatorSettings)
    {
        BlockSection emul('EMUL');

        u16 count = u16(m_editorOverlay.getWindow().getNumEditors());
        emul.poke16(1);

        // Number of editor files
        u16 realCount = count;
        for (u16 i = 0; i < count; ++i)
        {
            Editor& ed = m_editorOverlay.getWindow().getEditor(i);
            if (ed.getFileName().empty()) --realCount;
        }
        emul.poke16(realCount);

        // Number of labels
        const Labels& labels = m_debugger.getDisassemblyWindow().getLabels();
        emul.poke16((u16)labels.size());

        // Write out the editor file names
        for (u16 i = count; i > 0; --i)
        {
            Editor& ed = m_editorOverlay.getWindow().getEditor(i-1);
            string fn = ed.getFileName();
            if (!fn.empty()) emul.pokeString(fn);
        }

        // Write out the label information
        for (size_t i = 0; i < labels.size(); ++i)
        {
            emul.poke32(labels[i].second);
            emul.pokeString(labels[i].first);
        }

        f.addSection(emul, 0);
    }

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

void Nx::switchModel(Model model)
{
    m_emulator.switchModel(model);
    getSpeccy().reset(model);
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
// Editor/Assembler
//----------------------------------------------------------------------------------------------------------------------

void Nx::showEditor()
{
    m_editorOverlay.select();
}

void Nx::showDisassembler()
{
    m_disassemblerOverlay.select();
}

bool Nx::assemble(const vector<u8>& data, string sourceName)
{
    m_assemblerOverlay.select();
    m_assembler.startAssembly(data, sourceName);
    m_debugger.getDisassemblyWindow().setLabels(m_assembler.getLabels());
    m_editorOverlay.getWindow().setErrorInfos(m_assembler.getErrorInfos());

    return m_assembler.getErrorInfos().size() == 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------














































