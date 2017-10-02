//----------------------------------------------------------------------------------------------------------------------
// RAM and ROM emulation
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <vector>

class Memory
{
public:
    Memory(int clockScale);
    ~Memory();

    //
    // Contention
    //
    bool isContended(u16 addr);
    void contend(u16 addr, i64 t, int n, i64& inOutTStates);
    i64 contention(i64 tStates);

    //
    // ROM control
    //
    void setRomWriteState(bool writeable);

    //
    // Writing (poking)
    //
    void poke(u16 address, u8 b, i64& inOutTStates);        // 8-bit poke with contention
    void poke(u16 address, u8 b);                           // 8-bit poke without contention
    void poke16(u16 address, u16 w, i64& inOutTStates);     // 16-bit poke with contention
    void load(u16 address, const void* buffer, i64 size);   // Writes a buffer to memory, ignores ROM write state

    //
    // Reading (peeking)
    //
    u8 peek(u16 address, i64& inOutTStates);                // 8-bit peek with contention
    u8 peek(u16 address);                                   // 8-bit peek without contention
    u16 peek16(u16 address, i64& inOutTStates);             // 16-bit peek with contention

private:
    bool                m_romWritable;
    std::vector<u8>     m_memory;
    std::vector<u8>     m_contention;
};

#define LO(x) ((u8)((x) % 256))
#define HI(x) ((u8)((x) / 256))

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <random>
#include <algorithm>

#define KB(x) ((x) * 1024)

Memory::Memory(int clockScale)
    : m_romWritable(false)
    , m_memory(KB(64))
    , m_contention(70930 * clockScale)
{
    // Build contention table
    int contentionStart = 14335 * clockScale;
    int contentionEnd = contentionStart + (192 * 224 * clockScale);
    int t = contentionStart;

    while (t < contentionEnd)
    {
        // Calculate contention for the next 128 t-states (i.e. a single pixel line)
        for (int i = 0; i < 128; i += 8)
        {
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 6;
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 5;
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 4;
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 3;
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 2;
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 1;
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 0;
            for (int c = 0; c < clockScale; ++c) m_contention[t++] = 0;
        }

        // Skip the time the border is being drawn: 96 t-states (24 left, 24 right, 48 retrace)
        t += ((224 - 128) * clockScale);
    }

    // Fill up the memory with random bytes
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<int> dist(0, 255);
    for (int a = 0; a < 0xffff; ++a)
    {
        m_memory[a] = (u8)dist(rng);
    }
}

Memory::~Memory()
{
}

//----------------------------------------------------------------------------------------------------------------------
// Contention
//----------------------------------------------------------------------------------------------------------------------

bool Memory::isContended(u16 addr)
{
    return ((addr & 0xc000) == 0x4000);
}

void Memory::contend(u16 addr, i64 t, int n, i64& inOutTStates)
{
    if (isContended(addr))
    {
        for (int i = 0; i < n; ++i)
        {
            inOutTStates += contention(inOutTStates) + t;
        }
    }
    else
    {
        inOutTStates += t * n;
    }
}

i64 Memory::contention(i64 tStates)
{
    return m_contention[tStates];
}

//----------------------------------------------------------------------------------------------------------------------
// Poking
//----------------------------------------------------------------------------------------------------------------------

void Memory::poke(u16 address, u8 b)
{
    if (m_romWritable || address > -0x4000) m_memory[address] = b;
}

void Memory::poke(u16 address, u8 b, i64& inOutTStates)
{
    contend(address, 3, 1, inOutTStates);
    poke(address, b);
}

void Memory::poke16(u16 address, u16 w, i64& inOutTStates)
{
    poke(address, LO(w), inOutTStates);
    poke(address + 1, HI(w), inOutTStates);
}

void Memory::load(u16 address, const void* buffer, i64 size)
{
    i64 clampedSize = std::min((i64)address + (i64)size, (i64)KB(64)) - address;
    std::copy((const u8 *)buffer, (const u8 *)buffer + size, m_memory.begin() + address);
}

//----------------------------------------------------------------------------------------------------------------------
// Peeking
//----------------------------------------------------------------------------------------------------------------------

u8 Memory::peek(u16 address)
{
    return m_memory[address];
}

u8 Memory::peek(u16 address, i64& inOutTStates)
{
    contend(address, 3, 1, inOutTStates);
    return peek(address);
}

u16 Memory::peek16(u16 address, i64& inOutTStates)
{
    return peek(address, inOutTStates) + 256 * peek(address + 1, inOutTStates);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
