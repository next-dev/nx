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
// Events
//----------------------------------------------------------------------------------------------------------------------

class Machine;

class EventManager
{
public:
    class Event
    {
    public:
        // An event function to return false to exit the emulation loop so that the host can get updates.
        using Func = std::function<bool(Machine&, i64&)>;

        Event(i64 tState, std::string name, Func func)
            : m_tState(tState)
            , m_name(name)
            , m_func(func)
        {}

        bool trigger(Machine& M, i64& inOutTState)
        {
            return m_func(M, inOutTState);
        }

        i64 getTState() const { return m_tState; }
        const std::string& getName() const { return m_name; }
        Func getFunc() { return m_func; }

    private:
        i64             m_tState;
        std::string     m_name;
        Func            m_func;
    };

    void addEvent(i64 tState, std::string name, Event::Func func);
    bool testEvent(Machine& M, i64& inOutTState);

private:
    std::vector<Event>  m_events;
};

//----------------------------------------------------------------------------------------------------------------------
// Emulated Machine
//----------------------------------------------------------------------------------------------------------------------

class Machine
{
public:
    Machine(IHost& host, u32* img, std::vector<bool>& keys);

    // Returns the number of t-states that have passed.
    i64 update(i64& inOutTState);

    // Return various attributes of the machine
    int getClockScale() const { return m_clockScale; }
    int getFrameCounter() const { return m_frameCounter; }

    void setFrameCounter(int fc) { m_frameCounter = fc; }

    // Access to systems.
    EventManager& getEvents() { return m_events; }
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
    bool load(const u8* data, i64 size, FileType typeHint, i64& outTState);

private:
    bool loadSna(const u8* data, i64 size, i64& outTState);

private:
    IHost&          m_host;
    int             m_clockScale;
    Memory          m_memory;
    Io              m_io;
    Video           m_video;
    Z80             m_z80;
    int             m_frameCounter;
    EventManager    m_events;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// Event manager
//----------------------------------------------------------------------------------------------------------------------

void EventManager::addEvent(i64 tState, std::string name, Event::Func func)
{
    m_events.emplace_back(tState, name, func);

    // Sort the array
    std::sort(m_events.begin(), m_events.end(), [](Event& ev1, Event& ev2) -> bool {
        return ev1.getTState() < ev2.getTState();
    });
}

bool EventManager::testEvent(Machine& M, i64& inOutTState)
{
    bool result = true;

    // Keep looping until all passed events have been processed.
    while (result &&
        (m_events.size() > 0) &&
        (m_events[0].getTState() <= inOutTState))
    {
        if (!m_events[0].trigger(M, inOutTState)) result = false;
        m_events.erase(m_events.begin());
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------
// Events
//----------------------------------------------------------------------------------------------------------------------

bool machineFrame(Machine& M, i64& inOutTState)
{
    i64 time = 69888 * M.getClockScale();
    M.getEvents().addEvent(time, "frame", &machineFrame);
    inOutTState -= time;
    M.setFrameCounter(M.getFrameCounter() + 1);
    //M.getVideo().render((M.getFrameCounter() & 16) != 0);
    M.getHost().redraw();
    M.getZ80().interrupt();
    return false;
}

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
    int handle = m_host.load("48.rom", rom, size);
    if (handle)
    {
        m_memory.load(0, rom, size);
        m_host.unload(handle);

        // Bootstrap the events
        getEvents().addEvent(69888 * m_clockScale, "frame", &machineFrame);
    }
}

i64 Machine::update(i64& inOutTState)
{
    i64 elapsedTStates = 0;
    m_host.clear();

    while (getEvents().testEvent(*this, inOutTState))
    {
        i64 startTState = inOutTState;
        getZ80().step(inOutTState);
        elapsedTStates += (inOutTState - startTState);
        m_video.render((getFrameCounter() & 16) != 0, inOutTState);
    }

    return elapsedTStates;
}

//----------------------------------------------------------------------------------------------------------------------
// File loading
//----------------------------------------------------------------------------------------------------------------------

#define BYTE_OF(arr, offset) arr[offset]
#define WORD_OF(arr, offset) (*(u16 *)&arr[offset])

bool Machine::loadSna(const u8* data, i64 size, i64& outTState)
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

    m_z80.PC() = m_z80.pop(outTState);

    outTState = 0;

    return true;
}

#undef BYTE_OF
#undef WORD_OF

bool Machine::load(const u8* data, i64 size, FileType typeHint, i64& outTState)
{
    switch (typeHint)
    {
    case FileType::Sna:     return loadSna(data, size, outTState);
    default:
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
