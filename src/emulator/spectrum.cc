//----------------------------------------------------------------------------------------------------------------------
// Spectrum base class implementation
//----------------------------------------------------------------------------------------------------------------------

#include <emulator/spectrum.h>
#include <tape/tape.h>
#include <utils/format.h>

#include <algorithm>
#include <cassert>
#include <random>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Memory addressing
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// Banks
//----------------------------------------------------------------------------------------------------------------------

Bank::Bank(MemGroup group, u16 bank)
    : m_group(group)
    , m_bank(bank)
{

}

Bank::Bank(const Bank& other)
    : m_group(other.m_group)
    , m_bank(other.m_bank)
{

}

Bank& Bank::operator= (const Bank& other)
{
    m_group = other.m_group;
    m_bank = other.m_bank;
    return *this;
}

//----------------------------------------------------------------------------------------------------------------------
// MemAddr
//----------------------------------------------------------------------------------------------------------------------

MemAddr::MemAddr(Bank bank, u16 offset)
    : m_bank(bank)
    , m_offset(offset)
{

}

MemAddr::MemAddr(MemGroup group, int realAddress)
    : m_bank(group, u16(realAddress / kBankSize))
    , m_offset(u16(realAddress % kBankSize))
{

}

MemAddr::MemAddr(const MemAddr& memAddr)
    : m_bank(memAddr.m_bank)
    , m_offset(memAddr.m_offset)
{

}

MemAddr& MemAddr::operator= (const MemAddr& memAddr)
{
    m_bank = memAddr.bank();
    m_offset = memAddr.offset();
    return *this;
}

bool MemAddr::operator== (const MemAddr& addr) const
{
    return m_bank == addr.bank() && m_offset == addr.offset();
}

bool MemAddr::operator!= (const MemAddr& addr) const
{
    return m_bank != addr.bank() || m_offset != addr.offset();
}

bool MemAddr::operator< (const MemAddr& addr) const
{
    return m_bank < addr.bank() || m_offset < addr.offset();
}

bool MemAddr::operator> (const MemAddr& addr) const
{
    return m_bank > addr.bank() || m_offset > addr.offset();
}

bool MemAddr::operator<= (const MemAddr& addr) const
{
    return m_bank < addr.bank() ||
        (m_bank == addr.bank() && m_offset <= addr.offset());
}

bool MemAddr::operator>= (const MemAddr& addr) const
{
    return m_bank > addr.bank() ||
        (m_bank == addr.bank() && m_offset >= addr.offset());
}

MemAddr MemAddr::operator+ (int offset) const
{
    int i = index() + offset;
    return MemAddr(bank().getGroup(), i);
}

MemAddr MemAddr::operator- (int offset) const
{
    int i = index() - offset;
    return MemAddr(bank().getGroup(), i);
}

int MemAddr::operator- (MemAddr addr) const
{
    NX_ASSERT(bank().getGroup() == addr.bank().getGroup());
    return index() - addr.index();
}

MemAddr& MemAddr::operator++()
{
    if (++m_offset == kBankSize)
    {
        m_bank = Bank(m_bank.getGroup(), m_bank.getIndex() + 1);
        m_offset = 0;
    }

    return *this;
}

MemAddr MemAddr::operator++(int)
{
    MemAddr m = *this;
    operator++();
    return m;
}

MemAddr& MemAddr::operator--()
{
    if (m_offset-- == 0)
    {
        m_bank = Bank(m_bank.getGroup(), m_bank.getIndex() - 1);
        m_offset = kBankSize - 1;
    }

    return *this;
}

MemAddr MemAddr::operator--(int)
{
    MemAddr m = *this;
    operator--();
    return m;
}

//----------------------------------------------------------------------------------------------------------------------
// Z80MemAddr
//----------------------------------------------------------------------------------------------------------------------

Z80MemAddr::Z80MemAddr(u16 addr)
    : m_address(addr)
{

}

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Spectrum::Spectrum(function<void()> frameFunc)
    //--- Clock state ----------------------------------------------------
    : m_model(Model::ZX48)
    , m_tState(0)

    //--- Video state ----------------------------------------------------
    , m_image(new u32[kWindowWidth * kWindowHeight])
    , m_frameCounter(0)
    , m_videoWrite(0)
    , m_startTState(0)
    , m_drawTState(0)

    //--- Audio state ----------------------------------------------------
    , m_audio(69888, frameFunc)
    , m_tape(nullptr)

    //--- Memory state ---------------------------------------------------
    , m_romWritable(true)

    //--- CPU state ------------------------------------------------------
    , m_z80(*this)

    //--- ULA state ------------------------------------------------------
    , m_borderColour(7)
    , m_keys(8, 0)
    , m_speaker(0)
    , m_tapeEar(0)

    //--- 128K paging ----------------------------------------------------
    , m_pagingDisabled(false)
    , m_shadowScreen(false)

    //--- Breakpoints state ----------------------------------------------
    , m_break(false)

    //--- Kempston -------------------------------------------------------
    , m_kempstonJoystick(false)
    , m_kempstonState(0)
{
    reset(Model::ZX48);
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

void Spectrum::setRomWriteState(bool writable)
{
    m_romWritable = writable;
}

vector<MemAddr> Spectrum::findSequence(vector<u8> seq)
{
    auto it = m_ram.begin();
    vector<MemAddr> addresses;
    while (it != m_ram.end())
    {
        it = std::search(it, m_ram.end(), seq.begin(), seq.end());
        if (it != m_ram.end())
        {
            auto ramIndex = std::distance(m_ram.begin(), it);
            addresses.emplace_back(MemGroup::RAM, int(ramIndex));
            std::advance(it, seq.size());
        }
    }

    return addresses;
}

vector<MemAddr> Spectrum::findByte(u8 byte)
{
    return findSequence({ byte });
}

vector<MemAddr> Spectrum::findWord(u16 word)
{
    return findSequence({ u8(word % 256), u8(word / 256) });
}

vector<MemAddr> Spectrum::findString(string str)
{
    return findSequence(vector<u8>(str.begin(), str.end()));
}

MemAddr Spectrum::convertAddress(Z80MemAddr addr) const
{
    int slot = addr / kBankSize;
    int offset = addr % kBankSize;

    return MemAddr(m_slots[slot], offset);
}

Z80MemAddr Spectrum::convertAddress(MemAddr addr) const
{
    assert(addr.bank().getGroup() == MemGroup::RAM);
    auto it = find(m_slots.begin(), m_slots.end(), addr.bank());
    assert(it != m_slots.end());
    int slot = int(distance(m_slots.begin(), it));
    return Z80MemAddr(u16(slot * kBankSize + addr.offset()));
}

string Spectrum::addressName(MemAddr address)
{
    string s;

    // Memory format for RAM:
    //
    //  BB:AAAA     - B = 8-bit bank, A = 16-bit offset
    //  AAAA        - For 48K, A = 16-bit address (4000-ffff)
    //
    // Memory format for ROM:
    //
    // RX:AAAA      - X = Rom index
    // AAAA         - For 48K, A = 16-bit address (0000-3fff)
    //
    // For the Next, 16-bit offsets range from (0000-1fff).  Otherwise, (0000-3fff)
    //
    u8 bank = u8(address.bank().getIndex());
    u16 offset = u16(address.offset());

    switch (m_model)
    {
    case Model::ZX48:
        s = stringFormat("{0}", hexWord(convertAddress(address)));
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
        {
            switch (address.bank().getGroup())
            {
            case MemGroup::RAM:
                s = stringFormat("{0}:{1}",
                    hexByte(bank / 2),
                    hexWord(u16(0x2000)*(bank % 1) + offset));
                break;
            case MemGroup::ROM:
                s = stringFormat("R{0}:{1}",
                    hexNibble(bank / 2),
                    hexWord(u16(0x2000)*(bank % 1) + offset));
                break;
            default: assert(0);
            }
        }
        break;

    case Model::ZXNext:
        switch (address.bank().getGroup())
        {
        case MemGroup::RAM:
            s = stringFormat("{0}:{1}", hexByte(bank), hexWord(offset));
            break;

        case MemGroup::ROM:
            s = stringFormat("R{0}:{1}", hexByte(bank), hexWord(offset));
            break;

        default: assert(0);
        }
        break;

    default:
        assert(0);
    }

    return s;
}

//----------------------------------------------------------------------------------------------------------------------
// Overrides
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::reset(Model model)
{
    m_model = model;
    m_audio.stop();
    initMemory();
    initVideo();
    initIo();
    m_z80.restart();
    m_tState = 0;
    m_audio.start();
}

//----------------------------------------------------------------------------------------------------------------------
// Frame emulation
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::updateTape(TState numTStates)
{
    if (m_tape)
    {
        m_tapeEar = m_tape->play(numTStates);
    }
}

bool Spectrum::update(RunMode runMode, bool& breakpointHit)
{
    bool result = false;
    breakpointHit = false;
    TState frameTime = getFrameTime();
    TState startTState;

    switch (runMode)
    {
    case RunMode::Normal:
        while (m_tState < frameTime)
        {
            startTState = m_tState;
            m_z80.step(m_tState);
            updateVideo();
            updateTape(m_tState - startTState);
            m_audio.updateBeeper(m_tState, m_speaker, m_tapeEar ? 1 : 0);
            //m_audio.updateBeeper(m_tState, m_tapeEar ? 1 : 0);
            if ((runMode == RunMode::Normal) && (shouldBreak(convertAddress(Z80MemAddr(m_z80.PC()))) || m_break))
            {
                breakpointHit = true;
                m_break = false;
                break;
            }
        }
        break;

    case RunMode::StepIn:
    case RunMode::StepOver:
        startTState = m_tState;
        m_z80.step(m_tState);
        updateVideo();
        updateTape(m_tState - startTState);
        break;

    case RunMode::Stopped:
        // Do nothing
        break;
    }

    if (m_tState >= frameTime)
    {
        m_tState -= frameTime;
        m_z80.interrupt();
        result = true;
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------
// Memory
//----------------------------------------------------------------------------------------------------------------------

extern const u8 gRom48[16384];
extern const u8 gRom128_0[16384];
extern const u8 gRom128_1[16384];
extern const u8 gRomPlus2_0[16384];
extern const u8 gRomPlus2_1[16384];

void Spectrum::initMemory()
{
    setRomWriteState(true);
    m_slots.resize(8);

    switch (m_model)
    {
    case Model::ZX48:
        m_ram.resize(KB(48));
        m_rom.resize(KB(16));
        setSlot(0, Bank(MemGroup::ROM, 0));
        setSlot(1, Bank(MemGroup::ROM, 1));
        setSlot(2, Bank(MemGroup::RAM, 0));
        setSlot(3, Bank(MemGroup::RAM, 1));
        setSlot(4, Bank(MemGroup::RAM, 2));
        setSlot(5, Bank(MemGroup::RAM, 3));
        setSlot(6, Bank(MemGroup::RAM, 4));
        setSlot(7, Bank(MemGroup::RAM, 5));
        m_videoBank = m_shadowVideoBank = 2;
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
        m_ram.resize(KB(128));
        m_rom.resize(KB(32));
        setSlot(0, Bank(MemGroup::ROM, 2));
        setSlot(1, Bank(MemGroup::ROM, 3));
        setSlot(2, Bank(MemGroup::RAM, 10));
        setSlot(3, Bank(MemGroup::RAM, 11));
        setSlot(4, Bank(MemGroup::RAM, 4));
        setSlot(5, Bank(MemGroup::RAM, 5));
        setSlot(6, Bank(MemGroup::RAM, 0));
        setSlot(7, Bank(MemGroup::RAM, 1));
        m_videoBank = 10;
        m_shadowVideoBank = 14;
        break;

    case Model::ZXNext:
        m_ram.resize(kBankSize * 96);
        m_rom.resize(KB(16));
        setSlot(0, Bank(MemGroup::ROM, 0));
        setSlot(1, Bank(MemGroup::ROM, 1));
        setSlot(2, Bank(MemGroup::RAM, 10));
        setSlot(3, Bank(MemGroup::RAM, 11));
        setSlot(4, Bank(MemGroup::RAM, 4));
        setSlot(5, Bank(MemGroup::RAM, 5));
        setSlot(6, Bank(MemGroup::RAM, 0));
        setSlot(7, Bank(MemGroup::RAM, 1));
        m_videoBank = 10;
        m_shadowVideoBank = 14;
        break;
            
    default:
        assert(0);
        break;
    }
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
    for (int a = 0; a < m_ram.size(); ++a)
    {
        m_ram[a] = (u8)dist(rng);
    }

    // Initialise the ROMs
    switch (m_model)
    {
    case Model::ZX48:
        load(0, gRom48, KB(16));
        break;

    case Model::ZX128:
        // Load in the 128K ROM
        load(0, gRom128_1, KB(16));
        // Switch to 48K ROM and load
        setSlot(0, Bank(MemGroup::ROM, 0));
        setSlot(1, Bank(MemGroup::ROM, 1));
        load(0, gRom128_0, KB(16));
        break;

    case Model::ZXPlus2:
        // Load in the +2 ROM
        load(0, gRomPlus2_1, KB(16));
        // Switch to 48K ROM and load
        setSlot(0, Bank(MemGroup::ROM, 0));
        setSlot(1, Bank(MemGroup::ROM, 1));
        load(0, gRomPlus2_0, KB(16));
        break;

    case Model::ZXNext:
        load(0, gRom48, KB(16));
        break;

    default:
        assert(0);
    }
    setRomWriteState(false);
}

u8& Spectrum::memRef(MemAddr addr)
{
    vector<u8>& memGroup = getMemoryGroup(addr.bank().getGroup());
    return memGroup[addr.bank().getIndex() * kBankSize + addr.offset()];
}

u8 Spectrum::peek(Z80MemAddr address)
{
    MemAddr addr = convertAddress(address);
    return memRef(addr);
}

u8 Spectrum::peek(Z80MemAddr address, TState& t)
{
    contend(address, 3, 1, t);
    return peek(address);
}

u16 Spectrum::peek16(Z80MemAddr address, TState& t)
{
    return peek(address, t) + 256 * peek(address + 1, t);
}

void Spectrum::poke(Z80MemAddr address, u8 x)
{
    MemAddr addr = convertAddress(address);
    for (const auto& br : m_dataBreakpoints)
    {
        if ((addr >= br.address) && (addr < (br.address + br.len)))
        {
            m_break = true;
        }
    }

    if (m_romWritable || addr.bank().getGroup() == MemGroup::ROM)
    {
        memRef(addr) = x;
    }
}

void Spectrum::poke(Z80MemAddr address, u8 x, TState& t)
{
    contend(address, 3, 1, t);
    poke(address, x);
}

void Spectrum::poke16(Z80MemAddr address, u16 w, TState& t)
{
    Reg r(w);
    poke(address, r.l, t);
    poke(address + 1, r.h, t);
}

void Spectrum::load(Z80MemAddr address, const void* buffer, i64 size)
{
    MemAddr addr = convertAddress(address);
    vector<u8>& memGroup = getMemoryGroup(addr.bank().getGroup());

    copy((u8*)buffer, (u8*)buffer + size,
        memGroup.begin() + (addr.bank().getIndex() * kBankSize + addr.offset()));
}

void Spectrum::load(Z80MemAddr address, const vector<u8>& buffer)
{
    load(address, buffer.data(), buffer.size());
}

bool Spectrum::isContended(MemAddr addr) const
{
    // #todo: Cache the slot contention state on switch.  (Switch addr back to Z80MemAddr).
    Bank bank = addr.bank();
    MemGroup group = bank.getGroup();

    if (group == MemGroup::RAM)
    {
        int b = bank.getIndex();

        switch (getModel())
        {
        case Model::ZX48:
            return b < 2;

        case Model::ZX128:
        case Model::ZXPlus2:
            return (b & 2) == 2;

        case Model::ZXNext:
            return false;

        default: assert(0);
        }
    }
    return false;
}

bool Spectrum::isContended(u16 port) const
{
    return (port >= 0x4000 && port < 0x8000);
}

void Spectrum::contend(Z80MemAddr address, TState delay, int num, TState& t)
{
    MemAddr addr = convertAddress(address);
    if (isContended(addr))
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

void Spectrum::setSlot(int slot, Bank bank)
{
    assert(slot >= 0 && slot < 8);
    assert(bank.getIndex() >= 0 && bank.getIndex() < getNumBanks());
    m_slots[slot] = bank;

    // Generate the bank name
    string bn;
    switch (bank.getGroup())
    {
    case MemGroup::ROM: bn = "ROM"; break;
    case MemGroup::RAM: bn = "RAM"; break;
    default: assert(0);
    }

    m_bankNames[bank.getIndex()] += stringFormat(" {0}", bank.getIndex());
}

int Spectrum::getBank(int slot) const
{
    assert(slot >= 0 && slot < 8);
    return m_slots[slot].getIndex();
}

u16 Spectrum::getNumBanks() const
{
    return u16(m_ram.size() / kBankSize);
}

string& Spectrum::slotName(int slot)
{
    assert(slot >= 0 && slot < 8);
    return m_bankNames[m_slots[slot].getIndex()];
}

//----------------------------------------------------------------------------------------------------------------------
// I/O
//----------------------------------------------------------------------------------------------------------------------

void Spectrum::initIo()
{
    m_borderColour = 0;
    for (int i = 0; i < 8; ++i) m_keys[i] = 0;
    m_tapeEar = 0;
    m_speaker = 0;
    m_kempstonState = 0;
    m_pagingDisabled = false;
    m_shadowScreen = false;
}

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
        switch(p.l)
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

            setSlot(6, Bank(MemGroup::RAM, page * 2));
            setSlot(7, Bank(MemGroup::RAM, page * 2 + 1));
            m_shadowScreen = (shadow != 0);
            setSlot(0, Bank(MemGroup::ROM, rom ? 2 : 0));
            setSlot(1, Bank(MemGroup::ROM, rom ? 3 : 1));
            m_pagingDisabled = (disable != 0);
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
// Video
//----------------------------------------------------------------------------------------------------------------------

static const u16 kDoNotDraw = 0xffff;
static const u16 kBorder = 0xfffe;

// This needs to be called every time the video bank changes
void Spectrum::recalcVideoMaps()
{
    m_videoMap.resize(getFrameTime());

    // Start of display area is 14336.  We wait 4 t-states before we draw 8 pixels.  The left border is 24 pixels wide.
    // Each scan line is 224 t-states long
    m_startTState = (14340 - 24) - (224 * kBorderHeight);

    // Initialise the t-state->address map
    //
    // Values will be:
    //      0xffff  Do not draw
    //      0xfffe  Border colour
    //      0x0000+ Pixel address
    //
    int t = 0;

    // Vertical retrace period (and top border outside window)
    for (t = 0; t < m_startTState; ++t)
    {
        m_videoMap[t] = kDoNotDraw;
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
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = kDoNotDraw;
        for (int i = ta; i < 176 - ta; ++i) m_videoMap[t++] = kBorder;
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = kDoNotDraw;
    }

    // Build middle of display
    int x = 0, y = 0;
    int addr = 2;

    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224))
    {
        // Left border
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = kDoNotDraw;
        for (int i = ta; i < tb; ++i) m_videoMap[t++] = kBorder;

        // Screen line
        for (int i = tb; i < tb + 128; ++i)
        {
            // Every 4 t-states (8 pixels), we recalculate the address
            if (i % 4 == 0)
            {
                // Pixel offset is 000S SRRR CCCX XXXX, where Y = SSCCCRRR
                // Attr offset is  0001 10YY YYYX XXXX
                addr =
                    // Hi byte
                    // SS-------------+   RRR------+   
                    ((((y & 0xc0) >> 3) | (y & 0x07)) << 8) |
                    // Lo byte
                    // X-------------+   CCC-------------+
                    (((x >> 3) & 0x1f) | ((y & 0x38) << 2));
                x += 8;
            }
            m_videoMap[t++] = addr;
        }
        ++y;

        // Right border
        for (int i = tb + 128; i < 176 - ta; ++i) m_videoMap[t++] = kBorder;

        // Horizontal retrace + out of screen border
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = kDoNotDraw;
    }

    // Bottom border
    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224) + (kBorderHeight * 224))
    {
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = kDoNotDraw;
        for (int i = ta; i < 176 - ta; ++i) m_videoMap[t++] = kBorder;
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = kDoNotDraw;
    }

    while (t < getFrameTime())
    {
        m_videoMap[t++] = kDoNotDraw;
    }
}

void Spectrum::initVideo()
{
    m_videoTexture.create(kWindowWidth, kWindowHeight);
    m_videoSprite.setTexture(m_videoTexture);
    recalcVideoMaps();
}

void Spectrum::renderVideo()
{
    TState t = m_tState;
    m_tState = 69888;
    updateVideo();
    m_tState = t;
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
    int vramBank = m_model == Model::ZX48
        ? m_videoBank
        : m_shadowScreen ? m_shadowVideoBank : m_videoBank;

    for (int i = 0; i < numBytes; ++i)
    {
        if (m_videoMap[m_drawTState] < kBorder)
        {
            // Pixel address
            // #todo: refactor bank sizes to log-2?
            u16 paddr = m_videoMap[m_drawTState];
            u16 bank = (u16)vramBank + (paddr / kBankSize);
            u16 offset = paddr % kBankSize;
            u8 pixelData = memRef(MemAddr(Bank(MemGroup::RAM, bank), offset));

            // Calculate attribute address
            // 000S SRRR CCCX XXXX --> 0001 10SS CCCX XXXX
            u16 aaddr = ((paddr & 0x1800) >> 3) + (paddr & 0x00ff) + 0x1800;
            bank = (u16)vramBank + (aaddr / kBankSize);
            offset = aaddr % kBankSize;
            u8 attr = memRef(MemAddr(Bank(MemGroup::RAM, bank), offset));

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
        else if (m_videoMap[m_drawTState] == kBorder)
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

vector<Spectrum::Breakpoint>::const_iterator Spectrum::findBreakpoint(MemAddr address) const
{
    return find_if(m_breakpoints.begin(), m_breakpoints.end(),
        [address](const auto& br) -> bool {
            return br.address == address;
        });
}

vector<Spectrum::DataBreakpoint>::const_iterator Spectrum::findDataBreakpoint(MemAddr address, u16 len) const
{
    return find_if(m_dataBreakpoints.begin(), m_dataBreakpoints.end(),
        [address, len](const auto& br) -> bool {
            return br.address == address && br.len == len;
        });
}

void Spectrum::toggleBreakpoint(MemAddr address)
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

void Spectrum::toggleDataBreakpoint(MemAddr address, u16 len)
{
    auto it = findDataBreakpoint(address, len);
    if (it == m_dataBreakpoints.end())
    {
        m_dataBreakpoints.emplace_back(DataBreakpoint{ address, len });
    }
    else
    {
        m_dataBreakpoints.erase(it);
    }
}

void Spectrum::addTemporaryBreakpoint(MemAddr address)
{
    auto it = findBreakpoint(address);
    if (it == m_breakpoints.end())
    {
        m_breakpoints.emplace_back(Breakpoint{ BreakpointType::Temporary, address });
    }
}

bool Spectrum::shouldBreak(MemAddr address)
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

bool Spectrum::hasUserBreakpointAt(MemAddr address) const
{
    auto it = findBreakpoint(address);
    return (it != m_breakpoints.end() && it->type == BreakpointType::User);
}

bool Spectrum::hasDataBreakpoint(MemAddr address, u16 len) const
{
    auto it = findDataBreakpoint(address, len);
    return (it != m_dataBreakpoints.end());
}

vector<MemAddr> Spectrum::getUserBreakpoints() const
{
    vector<MemAddr> breakpoints;
    for (const auto& br : m_breakpoints)
    {
        if (br.type == BreakpointType::User)
        {
            breakpoints.push_back(br.address);
        }
    }
    return breakpoints;
}

void Spectrum::clearUserBreakpoints()
{
    decltype(m_breakpoints) newbps;
    copy_if(m_breakpoints.begin(), m_breakpoints.end(), back_inserter(newbps), 
        [](const auto& br) -> bool { return br.type != BreakpointType::User; });
    swap(m_breakpoints, newbps);
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
// Z80 Addressing
//----------------------------------------------------------------------------------------------------------------------

bool Spectrum::isZ80Address(MemAddr addr) const
{
    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        if (m_slots[i] == addr.bank()) return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
