//----------------------------------------------------------------------------------------------------------------------
//! Implementation of the Nx class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <audio/audio.h>
#include <emulator/nx.h>
#include <emulator/nxfile.h>
#include <emulator/spectrum.h>
#include <functional>
#include <utils/filename.h>
#include <utils/format.h>

#include <memory>

//----------------------------------------------------------------------------------------------------------------------
//! Nx's constructor

Nx::Nx(int argc, char** argv)
    : m_quit(false)
{
    //
    // Initialise the video
    //

    m_mainFrameState = FrameState(string("Nx Nova (") + NX_VERSION + ")", kDefaultScale * kWindowWidth, kDefaultScale * kWindowHeight);
    m_mainFrame.apply(m_mainFrameState);

    //
    // Initialise overlays
    //
    m_emulatorOverlay = make_shared<EmulatorOverlay>(*this);
    m_debuggerOverlay = make_shared<DebuggerOverlay>(*this);
    m_emulatorOverlay->apply(m_mainFrameState);
    m_debuggerOverlay->apply(m_mainFrameState);

    //
    // Initialise the machine
    //
    m_speccy.apply(Model::ZX48);


    setOverlay(m_emulatorOverlay, [] {});
}

//----------------------------------------------------------------------------------------------------------------------
//! Nx's destructor

Nx::~Nx()
{

}

//----------------------------------------------------------------------------------------------------------------------
// Main loop.

void Nx::run()
{
    while (!m_quit)
    {
        sf::Event event;
        enum class Skip
        {
            No,
            Yes,
            Maybe,
        }
        skipText = Skip::Maybe;

        //
        // Process the OS events
        //
        while (m_mainFrame.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::LostFocus:
                getEmulatorOverlay()->clearKeys();
                break;

            case sf::Event::GainedFocus:
                break;

            case sf::Event::Closed:
                m_quit = true;
                break;

            case sf::Event::KeyPressed:
                {
                    KeyEvent kev(event);
                    using K = sf::Keyboard::Key;

                    if (kev.isCtrl())
                    {
                        // Possible global key

                        switch (event.key.code)
                        {
                        case K::Num1:   setFrameDimensions(kWindowWidth * 2, kWindowHeight * 2);    break;
                        case K::Num2:   setFrameDimensions(kWindowWidth * 3, kWindowHeight * 3);    break;
                        case K::Num3:   setFrameDimensions(kWindowWidth * 4, kWindowHeight * 4);    break;

                        case K::R:
                            m_mainFrame.clearLayers();
                            getSpeccy().apply(getSpeccy().getModel());
                            rebuildLayers();
                            skipText = Skip::Yes;
                            break;

                        default:
                            break;
                        }
                    }
                    else if (kev.isNormal())
                    {
                        switch (event.key.code)
                        {
                        case K::Tilde:
                            if (!m_debuggerOverlay->isCurrent())
                            {
                                setOverlay(m_debuggerOverlay, [this] {
                                    setOverlay(m_emulatorOverlay, [] {});
                                });
                                skipText = Skip::Yes;
                            }
                            break;

                        default:
                            break;
                        }
                    }

                    if (skipText == Skip::Maybe)
                    {
                        skipText = getCurrentOverlay()->key(kev) ? Skip::Yes : Skip::No;
                    }
                }
                break;

            case sf::Event::KeyReleased:
                skipText = getCurrentOverlay()->key(KeyEvent(event)) ? Skip::Yes : Skip::No;
                break;

            case sf::Event::TextEntered:
                assert(skipText != Skip::Maybe);
                if (skipText == Skip::No)
                {
                    getCurrentOverlay()->text((char)event.text.unicode);
                    skipText = Skip::Maybe;
                }
                break;

            default:
                break;
            }
        }

        //
        // Generate a frame
        //
        if (!m_quit && getSpeccy().getAudio()->getSignal().isTriggered())
        {
            frame();
            m_mainFrame.render();
        }
    }

    // We can't have any references to an overlay since the overlay & its layer lifetime will be > SFML.
    Overlay::resetOverlays();
}

//----------------------------------------------------------------------------------------------------------------------

void Nx::setFrameDimensions(int width, int height)
{
    // Apply the change to the frame
    m_mainFrameState.width = width;
    m_mainFrameState.height = height;
    m_mainFrame.apply(m_mainFrameState);

    // Apply the changes to the overlays.
    m_emulatorOverlay->apply(m_mainFrameState);
    m_debuggerOverlay->apply(m_mainFrameState);

    rebuildLayers();
}

//----------------------------------------------------------------------------------------------------------------------

void Nx::frame()
{
    getSpeccy().update(RunMode::Normal);
}

//----------------------------------------------------------------------------------------------------------------------

void Nx::rebuildLayers()
{
    m_mainFrame.clearLayers();
    m_mainFrame.addLayer(getSpeccy().getVideoLayer());
    m_mainFrame.addLayer(static_pointer_cast<Layer>(getCurrentOverlay()));
    m_mainFrame.setScales();
}

//----------------------------------------------------------------------------------------------------------------------

void Nx::setOverlay(shared_ptr<Overlay> overlay, function<void()> onExit)
{
    Overlay::setOverlay(overlay, onExit);
    rebuildLayers();
}

//----------------------------------------------------------------------------------------------------------------------

bool Nx::openFile(string fileName)
{
    NX_ASSERT(!fileName.empty());

    // Get extension
    Path path = fileName;
    if (path.hasExtension())
    {
        string ext = path.extension();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".sna")
        {
            return loadSnaSnapshot(fileName);
        }
        else if (ext == ".z80")
        {
            return loadZ80Snapshot(fileName);
        }
        else if (ext == ".nx")
        {
            return loadNxSnapshot(fileName, true);
        }
        else
        {
            getCurrentOverlay()->error(stringFormat("Unknown file type in file '{0}'!", fileName));
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool Nx::loadSnaSnapshot(string fileName)
{
    vector<u8> buffer = NxFile::loadFile(fileName);
    u8* data = buffer.data();
    i64 size = (i64)buffer.size();

    if (size != 49179) return false;

    getSpeccy().apply(Model::ZX48);
    rebuildLayers();

    Z80& z80 = getSpeccy().getCpu();
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
    getSpeccy().setBorderColour(BYTE_OF(data, 26));
    getSpeccy().getMemorySystem().load(Bank(MemGroup::RAM, 0), data + 27, 0xc000);

    TState t = 0;
    z80.PC() = z80.pop(t);
    z80.IFF1() = z80.IFF2();
    getSpeccy().setTState(0);

    return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool Nx::loadZ80Snapshot(string fileName)
{
    vector<u8> buffer = NxFile::loadFile(fileName);
    u8* data = buffer.data();
    Z80& z80 = getSpeccy().getCpu();

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
    getSpeccy().setBorderColour((b12 & 0x0e) >> 1);
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
            TState t = 0;
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

                    getSpeccy().poke(a++, 0, t);
                }
                else if (b == 0xed)
                {
                    CHECK_BUFFER();
                    b = *mem++;
                    if (b != 0xed)
                    {
                        getSpeccy().poke(a++, 0xed, t);
                        getSpeccy().poke(a++, b, t);
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
                            getSpeccy().poke(a++, b, t);
                        }
                    }
                }
                else
                {
                    getSpeccy().poke(a++, b, t);
                }
            }
        }
        else
        {
            if (buffer.size() != (0xc000 + 30)) return false;

            getSpeccy().getMemorySystem().load(Bank(MemGroup::RAM, 0), data + 30, 0xc000);
        }
    }
    else
    {
        // Version 2 & 3 files
        u8* mem = data + 32 + WORD_OF(data, 30);
        z80.PC() = WORD_OF(data, 32);
        if (version == 3)
        {
            getSpeccy().setTState(TState(WORD_OF(data, 55)) + (TState(BYTE_OF(data, 57)) << 16));
        }

        u16 pages[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0xc000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000 };
        for (int i = 0; i < 3; ++i)
        {
            TState t = 0;
            u16 a = pages[BYTE_OF(mem, 2)];
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
                            getSpeccy().poke(a++, b, t);
                        }
                    }
                    else
                    {
                        getSpeccy().poke(a++, 0xed, t);
                        getSpeccy().poke(a++, b, t);
                    }
                }
                else
                {
                    getSpeccy().poke(a++, b, t);
                }
            }
            mem += len;
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool Nx::loadNxSnapshot(string fileName, bool full)
{
    getCurrentOverlay()->error("Not implemented!");
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

