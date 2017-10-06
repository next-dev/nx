//----------------------------------------------------------------------------------------------------------------------
// Common data structure for all machines
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "video.h"
#include "z80.h"

//----------------------------------------------------------------------------------------------------------------------
// Emulated Machine
//----------------------------------------------------------------------------------------------------------------------

class Machine
{
public:
    Machine(IHost& host, u32* img, std::vector<bool>& keys);

    void update();
    void restart();

    // Return various attributes of the machine
    int getClockScale() const { return m_clockScale; }
    int getFrameCounter() const { return m_frameCounter; }

    void setFrameCounter(int fc) { m_frameCounter = fc; }

    // Access to systems.
    Memory& getMemory() { return m_memory; }
    Io& getIo() { return m_io; }
    Video& getVideo() { return m_video; }
    Z80& getZ80() { return m_z80; }
    IHost& getHost() { return m_host; }

    // File loading.
    enum class FileType
    {
        Sna,
        Tap,
        Pzx,
    };
    bool load(const u8* data, i64 size, FileType typeHint);

private:
    bool loadSna(const u8* data, i64 size);

private:
    i64             m_tState;
    IHost&          m_host;
    int             m_clockScale;
    Memory          m_memory;
    Io              m_io;
    Video           m_video;
    Z80             m_z80;
    int             m_frameCounter;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <algorithm>

#ifdef __APPLE__
#include "ResourcePath.hpp"
#endif

//----------------------------------------------------------------------------------------------------------------------
// Machine
//----------------------------------------------------------------------------------------------------------------------

Machine::Machine(IHost& host, u32* img, std::vector<bool>& keys)
    : m_host(host)
    , m_clockScale(1)
    , m_memory(1)
    , m_io(m_memory, keys)
    , m_video(m_memory, m_io, img)
    , m_z80(m_memory, m_io)
    , m_frameCounter(0)
{
    // Load the ROM into memory.
    const u8* rom;
    i64 size;
#ifdef __APPLE__
    int handle = m_host.load(resourcePath() + "48.rom", rom, size);
#else
    int handle = m_host.load("48.rom", rom, size);
#endif
    if (handle)
    {
        m_memory.load(0, rom, size);
        m_host.unload(handle);
        m_video.frame();
    }
}

void Machine::update()
{
    m_tState = 0;

    i64 frameTime = 69888 * getClockScale();

    while (m_tState < frameTime)
    {
        getZ80().step(m_tState);
        m_video.render((getFrameCounter() & 16) != 0, m_tState);
    }

    m_tState -= frameTime;
    setFrameCounter(getFrameCounter() + 1);
    getVideo().frame();
    getZ80().interrupt();
}

void Machine::restart()
{
    m_tState = 0;
    setFrameCounter(0);
    getVideo().frame();
    getZ80().restart();
}

//----------------------------------------------------------------------------------------------------------------------
// File loading
//----------------------------------------------------------------------------------------------------------------------

#define BYTE_OF(arr, offset) arr[offset]
#define WORD_OF(arr, offset) (*(u16 *)&arr[offset])

bool Machine::loadSna(const u8* data, i64 size)
{
    if (size != 49179) return false;

    m_z80.I() = BYTE_OF(data, 0);
    m_z80.HL_() = WORD_OF(data, 1);
    m_z80.DE_() = WORD_OF(data, 3);
    m_z80.BC_() = WORD_OF(data, 5);
    m_z80.AF_() = WORD_OF(data, 7);
    m_z80.HL() = WORD_OF(data, 9);
    m_z80.DE() = WORD_OF(data, 11);
    m_z80.BC() = WORD_OF(data, 13);
    m_z80.IX() = WORD_OF(data, 15);
    m_z80.IY() = WORD_OF(data, 17);
    m_z80.IFF1() = (BYTE_OF(data, 19) & 0x01) != 0;
    m_z80.IFF2() = (BYTE_OF(data, 19) & 0x04) != 0;
    m_z80.R() = BYTE_OF(data, 20);
    m_z80.AF() = WORD_OF(data, 21);
    m_z80.SP() = WORD_OF(data, 23);
    m_z80.IM() = BYTE_OF(data, 25);
    m_io.setBorder(BYTE_OF(data, 26));
    m_memory.load(0x4000, data + 27, 0xc000);

    m_z80.PC() = m_z80.pop(m_tState);

    m_tState = 0;

    return true;
}

#undef BYTE_OF
#undef WORD_OF

bool Machine::load(const u8* data, i64 size, FileType typeHint)
{
    switch (typeHint)
    {
    case FileType::Sna:     return loadSna(data, size);
    default:
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
