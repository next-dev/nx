//----------------------------------------------------------------------------------------------------------------------
//! Implements the Spectrum class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <audio/audio.h>
#include <emulator/spectrum.h>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// S P E C T R U M
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

Spectrum::Spectrum()
    : m_tState(0)
    , m_z80(*this)
    , m_frameCounter(0)
    , m_keys(8, 0)
    , m_speaker(0)
    , m_tapeEar(0)
    , m_pagingDisabled(false)
    , m_shadowScreen(false)
    , m_kempstonJoystick(false)
    , m_kempstonState(0)
{
}

//----------------------------------------------------------------------------------------------------------------------

Spectrum::~Spectrum()
{

}

//----------------------------------------------------------------------------------------------------------------------

extern const u8 gRom48[16384];
extern const u8 gRom128_0[16384];
extern const u8 gRom128_1[16384];
extern const u8 gRomPlus2_0[16384];
extern const u8 gRomPlus2_1[16384];

void Spectrum::apply(Model model)
{
    // Stop emulation if it is running.
    if (m_audio)
    {
        m_audio->stop();
    }

    //
    // Apply the memory model
    //

    m_model = model;

    switch (model)
    {
    case Model::ZX48:
        m_memorySystem.applySizes({ 2, 6 });
        m_memorySystem.load(Bank(MemGroup::ROM, 0), gRom48, KB(16));
        m_slots = Memory::Slots {
            Bank { MemGroup::ROM, 0 },
            Bank { MemGroup::ROM, 1 },
            Bank { MemGroup::RAM, 0 },
            Bank { MemGroup::RAM, 1 },
            Bank { MemGroup::RAM, 2 },
            Bank { MemGroup::RAM, 3 },
            Bank { MemGroup::RAM, 4 },
            Bank { MemGroup::RAM, 5 },
        };
        m_videoState.videoBank = Bank{ MemGroup::RAM, 0 };
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
        m_memorySystem.applySizes({ 4, 16 });
        if (model == Model::ZX128)
        {
            m_memorySystem.load(Bank(MemGroup::ROM, 0), gRom128_0, KB(16));
            m_memorySystem.load(Bank(MemGroup::ROM, 2), gRom128_1, KB(16));
        }
        else
        {
            m_memorySystem.load(Bank(MemGroup::ROM, 0), gRomPlus2_0, KB(16));
            m_memorySystem.load(Bank(MemGroup::ROM, 2), gRomPlus2_1, KB(16));
        }
        m_slots = Memory::Slots {
            Bank { MemGroup::ROM, 0 },
            Bank { MemGroup::ROM, 1 },
            Bank { MemGroup::RAM, 10 },
            Bank { MemGroup::RAM, 11 },
            Bank { MemGroup::RAM, 4 },
            Bank { MemGroup::RAM, 5 },
            Bank { MemGroup::RAM, 0 },
            Bank { MemGroup::RAM, 1 },
        };
        m_videoState.videoBank = Bank{ MemGroup::RAM, 10 };
        break;

    case Model::ZXNext:
        m_memorySystem.applySizes({ 2, 96 });
        m_memorySystem.load(Bank(MemGroup::ROM, 0), gRom48, KB(16));
        m_slots = Memory::Slots{
            Bank { MemGroup::ROM, 0 },
            Bank { MemGroup::ROM, 1 },
            Bank { MemGroup::RAM, 10 },
            Bank { MemGroup::RAM, 11 },
            Bank { MemGroup::RAM, 4 },
            Bank { MemGroup::RAM, 5 },
            Bank { MemGroup::RAM, 0 },
            Bank { MemGroup::RAM, 1 },
        };
        m_videoState.videoBank = Bank{ MemGroup::RAM, 10 };
        break;

    default:
        NX_ASSERT(0);
    }

    m_memorySystem.applySlots(m_slots);

    //
    // Generate the contentions
    //

    m_contention.resize(70930, 0);
    int contentionStart = 14335;
    int contentionEnd = contentionStart + (192 * 224);
    int t = contentionStart;

    while (t < contentionEnd)
    {
        // Calculate contention for the next 128 t-states (i.e. a single pixel line)
        for (int i = 0; i < 128; i += 8)
        {
            m_contention[t++] = 6;
            m_contention[t++] = 5;
            m_contention[t++] = 4;
            m_contention[t++] = 3;
            m_contention[t++] = 2;
            m_contention[t++] = 1;
            m_contention[t++] = 0;
            m_contention[t++] = 0;
        }

        // Skip the time the border is being drawn: 96 t-states (24 left, 24 right, 48 retrace)
        t += (224 - 128);
    }

    //
    // Set up the video layer
    //
    m_videoLayer = make_shared<UlaLayer>(m_memorySystem);
    LayerState layerState(kWindowWidth, kWindowHeight);
    m_videoLayer->Layer::apply(layerState);
    m_videoLayer->apply(m_videoState);

    //
    // Reset the Z80
    //
    m_z80.restart();

    //
    // Set up the audio layer
    //
    // #todo: Change the audio API to use apply.
    m_audio = make_unique<Audio>(kFrameTStates);
    m_audio->start();
}

//----------------------------------------------------------------------------------------------------------------------

bool Spectrum::update(RunMode runMode)
{
    bool result = false;
    TState startTState;

    switch (runMode)
    {
    case RunMode::Normal:
        while (m_tState < kFrameTStates)
        {
            startTState = m_tState;
            m_z80.step(m_tState);
            m_videoState.tStates = m_tState;
            m_videoLayer->apply(m_videoState);
            m_videoLayer->update();
            m_audio->updateBeeper(m_tState, m_speaker, m_tapeEar ? 1 : 0);
        }
        break;

    case RunMode::StepIn:
    case RunMode::StepOver:
        startTState = m_tState;
        m_z80.step(m_tState);
        m_videoState.tStates = m_tState;
        m_videoLayer->apply(m_videoState);
        m_videoLayer->update();
        break;

    case RunMode::Stopped:
        // Do nothing
        break;
    }


    if (m_tState >= kFrameTStates)
    {
        m_tState -= kFrameTStates;
        m_z80.interrupt();
        result = true;
        ++m_frameCounter;
        m_videoState.flash = m_frameCounter & 16;
        result = true;
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::renderVRAM()
{
    //m_videoLayer->render();
}

//----------------------------------------------------------------------------------------------------------------------
// IExternals interface

u8 Spectrum::peek(u16 address)
{
    return m_memorySystem.peek8(address);
}

//----------------------------------------------------------------------------------------------------------------------

u8 Spectrum::peek(u16 address, TState& t)
{
    contend(address, 3, 1, t);
    return peek(address);
}

//----------------------------------------------------------------------------------------------------------------------

u16 Spectrum::peek16(u16 address, TState& t)
{
    return (u16)peek(address, t) + 256 * (u16)peek(address + 1, t);
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::poke(u16 address, u8 x, TState& t)
{
    // #todo: check for data breakpoints

    contend(address, 3, 1, t);
    m_memorySystem.poke8(address, x);
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::poke16(u16 address, u16 x, TState& t)
{
    Reg r(x);
    poke(address, r.l, t);
    poke(address + 1, r.h, t);
}

//----------------------------------------------------------------------------------------------------------------------

bool Spectrum::isContended(MemAddr addr) const
{
    Bank bank = addr.bank();
    MemGroup group = bank.group();

    if (group == MemGroup::RAM)
    {
        int b = bank.index();

        switch (m_model)
        {
        case Model::ZX48:       return b < 2;
        case Model::ZX128:
        case Model::ZXPlus2:    return (b & 1) == 1;
        case Model::ZXNext:     return false;

        default:
            NX_ASSERT(0);
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool Spectrum::isContended(u16 port) const
{
    return (port >= 0x4000 && port < 0x8000);
}

//----------------------------------------------------------------------------------------------------------------------

TState Spectrum::contention(TState t) const
{
    return m_contention[t];
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::contend(u16 address, TState delay, int num, TState& t)
{
    MemAddr a = m_memorySystem.convert(address);
    if (isContended(a))
    {
        for (int i = 0; i < num; ++i) t += contention(t) + delay;
    }
    else
    {
        t += delay * num;
    }
}

//----------------------------------------------------------------------------------------------------------------------

u8 Spectrum::in(u16 port, TState& t)
{
    u8 x = 0xff;
    bool isUla48Port = ((port & 1) == 0);

    //
    // Early contention
    //
    if (isContended(port))
    {
        contend(port, 1, 1, t);
    }
    else
    {
        ++t;
    }

    //
    // Late contention
    //
    if (isUla48Port)
    {
        contend(port, 3, 1, t);
    }
    else
    {
        if (isContended(port))
        {
            contend(port, 1, 3, t);
        }
        else
        {
            t += 3;
        }
    }

    //
    // Fetch the actual value from the port
    //

    Reg p(port);
    if (isUla48Port)
    {
        x = 0xff;
        u8 row = p.h;
        for (int i = 0; i < 8; ++i)
        {
            if ((row & 1) == 0)
            {
                // This row is required to be read
                x &= ~m_keys[i];
            }
            row >>= 1;
        }

        x = (x & 0xbf) | m_tapeEar;
    }
    else
    {
        switch (p.l)
        {
        case 0x1f:
            // Kempston joystick
            x = m_kempstonState;
            break;

        default:
            // Do nothing!
            // TODO: Floating bus!
            break;
        }
    }
    return x;
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::out(u16 port, u8 x, TState& t)
{
    //
    // Early contention
    //
    if (isContended(port))
    {
        contend(port, 1, 1, t);
    }
    else
    {
        ++t;
    }

    bool isUlaPort = ((port & 1) == 0);

    //
    // Deal with the port
    //
    if (isUlaPort)
    {
        m_videoState.borderColour = x & 7;
        m_videoLayer->apply(m_videoState);
        m_speaker = (x & 0x10) ? 1 : 0;
    }

    //
    // 128K ports
    //
    if (m_model == Model::ZX128 ||
        m_model == Model::ZXPlus2)
    {
        if (!m_pagingDisabled && (port & 0x8002) == 0)
        {
            u8 page = x & 0x07;
            u8 shadow = x & 0x08;
            u8 rom = x & 0x10;
            u8 disable = x & 0x20;

            m_slots[0] = Bank(MemGroup::ROM, rom ? 2 : 0);
            m_slots[1] = Bank(MemGroup::ROM, rom ? 3 : 1);
            m_slots[6] = Bank(MemGroup::RAM, page * 2);
            m_slots[7] = Bank(MemGroup::RAM, page * 2 + 1);
            m_shadowScreen = (shadow != 0);
            m_pagingDisabled = (disable != 0);

            getMemorySystem().applySlots(m_slots);
        }
    }

    //
    // Late contention
    //
    if (isUlaPort)
    {
        contend(port, 3, 1, t);
    }
    else
    {
        if (isContended(port))
        {
            contend(port, 1, 3, t);
        }
        else
        {
            t += 3;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::applyKeyboard(vector<u8>&& keys)
{
    m_keys = move(keys);
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::setBorderColour(u8 colour)
{
    m_videoState.borderColour = colour;
}

//----------------------------------------------------------------------------------------------------------------------

void Spectrum::setTState(TState t)
{
    m_tState = t;
}

//----------------------------------------------------------------------------------------------------------------------
