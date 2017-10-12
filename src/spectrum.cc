//----------------------------------------------------------------------------------------------------------------------
// Spectrum base class implementation
//----------------------------------------------------------------------------------------------------------------------

#include "spectrum.h"

#include <algorithm>
#include <cassert>
#include <random>

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Spectrum::Spectrum()
    //--- Clock state ----------------------------------------------------
    : m_tState(0)

    //--- Video state ----------------------------------------------------
    , m_image(new u32[kWindowWidth * kWindowHeight])
    , m_frameCounter(0)
    , m_videoWrite(0)
    , m_startTState(0)
    , m_drawTState(0)

    //--- Memory state ---------------------------------------------------
    , m_romWritable(true)

    //--- CPU state ------------------------------------------------------
    , m_z80(*this)

    //--- ULA state ------------------------------------------------------
    , m_borderColour(7)

    //--- Kempston -------------------------------------------------------
    , m_kempstonJoystick(false)
    , m_kempstonState(0)
{
    reset();
}

//----------------------------------------------------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------------------------------------------------

Spectrum::~Spectrum()
{
    delete[] m_image;
}

//----------------------------------------------------------------------------------------------------------------------
// State
//----------------------------------------------------------------------------------------------------------------------

sf::Sprite& Spectrum::getVideoSprite()
{
    m_videoTexture.update((const sf::Uint8 *)m_image);
    return m_videoSprite;
}

void Spectrum::setKeyboardState(vector<u8> &rows)
{
    m_keys = rows;
}

void Spectrum::setBorderColour(u8 borderColour)
{
    m_borderColour = borderColour & 7;
}

//----------------------------------------------------------------------------------------------------------------------
// Overrides
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::reset(bool hard /* = true */)
{
    if (hard)
    {
        initMemory();
        initVideo();
    }
    m_z80.restart();
    m_tState = 0;
}

//----------------------------------------------------------------------------------------------------------------------
// Frame emulation
//----------------------------------------------------------------------------------------------------------------------

bool Spectrum::update(RunMode runMode, bool& breakpointHit)
{
    bool result = false;
    breakpointHit = false;
    TState frameTime = getFrameTime();

    switch (runMode)
    {
    case RunMode::Normal:
        while (m_tState < getFrameTime())
        {
            m_z80.step(m_tState);
            updateVideo();
            if ((runMode == RunMode::Normal) && shouldBreak(m_z80.PC()))
            {
                breakpointHit = true;
                break;
            }
        }
        break;

    case RunMode::StepIn:
    case RunMode::StepOver:
        m_z80.step(m_tState);
        updateVideo();
        break;

    case RunMode::Stopped:
        // Do nothing
        break;
    }

    if (m_tState >= getFrameTime())
    {
        m_tState -= getFrameTime();
        m_z80.interrupt();
        result = true;
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------
// Memory
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::initMemory()
{
    m_ram.resize(65536);
    m_contention.resize(70930);

    // Build contention table
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

    // Fill up the memory with random bytes
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<int> dist(0, 255);
    for (int a = 0; a < 0xffff; ++a)
    {
        m_ram[a] = (u8)dist(rng);
    }
}

u8 Spectrum::peek(u16 address)
{
    return m_ram[address];
}

u8 Spectrum::peek(u16 address, TState& t)
{
    contend(address, 3, 1, t);
    return peek(address);
}

u16 Spectrum::peek16(u16 address, TState& t)
{
    return peek(address, t) + 256 * peek(address + 1, t);
}

void Spectrum::poke(u16 address, u8 x)
{
    if (m_romWritable || address >= 0x4000) m_ram[address] = x;
}

void Spectrum::poke(u16 address, u8 x, TState& t)
{
    contend(address, 3, 1, t);
    poke(address, x);
}

void Spectrum::poke16(u16 address, u16 w, TState& t)
{
    Reg r(w);
    poke(address, r.l, t);
    poke(address + 1, r.h, t);
}

void Spectrum::load(u16 address, const void* buffer, i64 size)
{
    i64 clampedSize = min((i64)address + size, (i64)65536) - address;
    copy((u8*)buffer, (u8*)buffer + size, m_ram.begin() + address);
}

void Spectrum::load(u16 address, const vector<u8>& buffer)
{
    load(address, buffer.data(), buffer.size());
}

bool Spectrum::isContended(u16 addr) const
{
    return ((addr & 0xc000) == 0x4000);
}

void Spectrum::contend(u16 address, TState delay, int num, TState& t)
{
    if (isContended(address))
    {
        for (int i = 0; i < num; ++i)
        {
            t += contention(t) + delay;
        }
    }
    else
    {
        t += delay * num;
    }
}

TState Spectrum::contention(TState tStates)
{
    return m_contention[tStates];
}

//----------------------------------------------------------------------------------------------------------------------
// I/O
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::ioContend(u16 port, TState delay, int num, TState& t)
{
    if (isContended(port))
    {
        for (int i = 0; i < num; ++i)
        {
            t += contention(t) + delay;
        }
    }
    else
    {
        t += delay * num;
    }
}

u8 Spectrum::in(u16 port, TState& t)
{
    u8 x = 0;
    bool isUlaPort = ((port & 1) == 0);

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

    //
    // Fetch the actual value from the port
    //

    Reg p(port);
    if (isUlaPort)
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
    }
    else
    {
        switch(p.l)
        {
        case 0x1f:
            // Kempston joystick
            x = m_kempstonState;
            break;
                
        default:
            // Do nothing!
            break;
        }
    }
    return x;
}

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
        m_borderColour = x & 7;
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
// Video
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::initVideo()
{
    m_videoTexture.create(kWindowWidth, kWindowHeight);
    m_videoSprite.setTexture(m_videoTexture);
    m_videoSprite.setScale(4, 4);

    m_videoMap.resize(getFrameTime());

    // Start of display area is 14336.  We wait 4 t-states before we draw 8 pixels.  The left border is 24 pixels wide.
    // Each scan line is 224 t-states long
    m_startTState = (14340 - 24) - (224 * kBorderHeight);

    // Initialise the t-state->address map
    //
    // Values will be:
    //      0       Do not draw
    //      1       Border colour
    //      0x4000+ Pixel address
    //
    int t = 0;

    // Vertical retrace period (and top border outside window)
    for (t = 0; t < m_startTState; ++t)
    {
        m_videoMap[t] = 0;
    }

    // Calculate line timings
    //
    // +---------- TV width ------------------+
    // |   +------ Window width ----------+   |
    // |   |  +--- Screen width -------+  |   |
    // v   v  v                        v  v   v
    // +---+--+------------------------+--+---+-----+
    // |000|11|aaaaaaaaaaaaaaaaaaaaaaaa|11|000|00000|
    // +---+--+------------------------+--+---+-----+
    //     ta tb                          176-ta    224
    //                                 176-tb
    int ta = (kTvWidth - kWindowWidth) / 4;
    int tb = (kTvWidth - kScreenWidth) / 4;

    // Top border
    while (t < (m_startTState + (kBorderHeight * 224)))
    {
        // The line is: border (w/2t), screen (128t), border (w/2t) = 128+wt
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = 0;
        for (int i = ta; i < 176 - ta; ++i) m_videoMap[t++] = 1;
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = 0;
    }

    // Build middle of display
    int x = 0, y = 0;
    int addr = 2;

    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224))
    {
        // Left border
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = 0;
        for (int i = ta; i < tb; ++i) m_videoMap[t++] = 1;

        // Screen line
        for (int i = tb; i < tb + 128; ++i)
        {
            // Every 4 t-states (8 pixels), we recalculate the address
            if (i % 4 == 0)
            {
                // Pixel address is 010S SRRR CCCX XXXX, where Y = SSCCCRRR
                // Attr address is  0101 10YY YYYX XXXX
                addr =
                    // Hi byte
                    // SS-------------+   RRR------+   
                    ((((y & 0xc0) >> 3) | (y & 0x07) | (0x40)) << 8) |
                    // Lo byte
                    // X-------------+   CCC-------------+
                    (((x >> 3) & 0x1f) | ((y & 0x38) << 2));
                x += 8;
            }
            m_videoMap[t++] = addr;
        }
        ++y;

        // Right border
        for (int i = tb + 128; i < 176 - ta; ++i) m_videoMap[t++] = 1;

        // Horizontal retrace + out of screen border
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = 0;
    }

    // Bottom border
    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224) + (kBorderHeight * 224))
    {
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = 0;
        for (int i = ta; i < 176 - ta; ++i) m_videoMap[t++] = 1;
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = 0;
    }

    while (t < getFrameTime())
    {
        m_videoMap[t++] = 0;
    }
}

void Spectrum::updateVideo()
{
    bool flash = (m_frameCounter & 16) != 0;
    TState tState = m_tState;

    static const u32 colours[16] =
    {
        0xff000000, 0xffd70000, 0xff0000d7, 0xffd700d7, 0xff00d700, 0xffd7d700, 0xff00d7d7, 0xffd7d7d7,
        0xff000000, 0xffff0000, 0xff0000ff, 0xffff00ff, 0xff00ff00, 0xffffff00, 0xff00ffff, 0xffffffff,
    };

    // Nothing to draw yet
    if (tState < m_startTState) return;
    if (tState >= getFrameTime())
    {
        tState = getFrameTime()-1;
    }

    // It takes 4 t-states to write 1 byte.
    int elapsedTStates = int(tState + 1 - m_drawTState);
    int numBytes = (elapsedTStates >> 2) + ((elapsedTStates % 4) > 0 ? 1 : 0);

    for (int i = 0; i < numBytes; ++i)
    {
        if (m_videoMap[m_drawTState] > 1)
        {
            // Pixel address
            u16 paddr = m_videoMap[m_drawTState];
            u8 pixelData = peek(paddr);

            // Calculate attribute address
            // 010S SRRR CCCX XXXX --> 0101 10SS CCCX XXXX
            u16 aaddr = ((paddr & 0x1800) >> 3) + (paddr & 0x00ff) + 0x5800;
            u8 attr = peek(aaddr);

            u8 lastPixelData = pixelData;
            u8 lastAttrData = attr;

            // Bright is either 0x08 or 0x00
            u8 bright = (attr & 0x40) >> 3;
            u8 ink = (attr & 0x07);
            u8 paper = (attr & 0x38) >> 3;
            u32 c0 = colours[paper + bright];
            u32 c1 = colours[ink + bright];

            if (flash && (attr & 0x80))
            {
                std::swap(c0, c1);
            }

            // We have a byte of pixel data and a byte of attr data - draw 8 pixels
            for (int p = 0; p < 8; ++p)
            {
                if ((pixelData & 0x80) != 0)
                {
                    assert(m_videoWrite < (kWindowWidth * kWindowHeight));
                    m_image[m_videoWrite++] = c1;
                    lastAttrData = ink;
                }
                else
                {
                    assert(m_videoWrite < (kWindowWidth * kWindowHeight));
                    m_image[m_videoWrite++] = c0;
                    lastAttrData = paper;
                }
                pixelData <<= 1;
            }
        } // pixel data
        else if (m_videoMap[m_drawTState] == 1)
        {
            u32 border = colours[getBorderColour()];
            for (int b = 0; b < 8; ++b)
            {
                assert(m_videoWrite < (kWindowWidth * kWindowHeight));
                m_image[m_videoWrite++] = border;
            }
        }

        m_drawTState += 4;
    } // for numbytes

    if (m_tState >= getFrameTime())
    {
        m_videoWrite = 0;
        m_drawTState = m_startTState;
        ++m_frameCounter;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Breakpoints
//----------------------------------------------------------------------------------------------------------------------

vector<Spectrum::Breakpoint>::iterator Spectrum::findBreakpoint(u16 address)
{
    for (auto it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    {
        if (it->address == address) return it;
    }

    return m_breakpoints.end();
}

void Spectrum::toggleBreakpoint(u16 address)
{
    auto it = findBreakpoint(address);
    if (it == m_breakpoints.end())
    {
        m_breakpoints.emplace_back(Breakpoint{ BreakpointType::User, address });
    }
    else
    {
        m_breakpoints.erase(it);
    }
}

void Spectrum::addTemporaryBreakpoint(u16 address)
{
    auto it = findBreakpoint(address);
    if (it == m_breakpoints.end())
    {
        m_breakpoints.emplace_back(Breakpoint{ BreakpointType::Temporary, address });
    }
}

bool Spectrum::shouldBreak(u16 address)
{
    if (m_breakpoints.size() == 0) return false;
    auto it = findBreakpoint(address);
    bool result = it != m_breakpoints.end();
    if (result && (it->type == BreakpointType::Temporary))
    {
        m_breakpoints.erase(it);
    }

    return result;
}

bool Spectrum::hasUserBreakpointAt(u16 address)
{
    auto it = findBreakpoint(address);
    return (it != m_breakpoints.end() && it->type == BreakpointType::User);
}

//----------------------------------------------------------------------------------------------------------------------
// Kempston Joystick emulation
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::setKempstonState(u8 state)
{
    m_kempstonState = state;
}

u8 Spectrum::getKempstonState() const
{
    return m_kempstonState;
}


//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
