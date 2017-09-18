//----------------------------------------------------------------------------------------------------------------------
// RAM and ROM emulation
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/k_platform.h>
#include <kore/k_memory.h>

//----------------------------------------------------------------------------------------------------------------------
// Memory state
//----------------------------------------------------------------------------------------------------------------------

typedef struct 
{
    u8*         memory;
    u8*         contention;
}
Memory;

typedef struct 
{
    u8*         r;      // Read reference
    u8*         w;      // Write reference
}
Ref;

// Initialise the memory.  If it fails, mem will be left in null state.
bool memoryOpen(Memory* mem);

// Close the memory sub-system
void memoryClose(Memory* mem);

// Return the number of t-states of contention for a particular memory address at a particular t-state time from
// video trace.
i64 memoryContention(Memory* mem, u16 addr, i64 tStates);

// Set an 8-bit value to the memory.  Will not write if the memory address points to ROM.  Will also adjust the
// tState counter according to contention.
void memoryPoke(Memory* mem, u16 address, u8 b, i64* inOutTStates);

// Set a 16-bit value to the memory in little endian form.  Will also adjust the tState counter according to
// contention.
void memoryPoke16(Memory* mem, u16 address, u16 w, i64* inOutTStates);

// Read an 8-bit value from the memory.
u8 memoryPeek(Memory* mem, u16 address, i64* inOutTStates);

// Read an 16-bit value from the memory.
u16 memoryPeek16(Memory* mem, u16 address, i64* inOutTStates);

// Get a reference to a memory address.
Ref memoryRef(Memory* mem, u16 address, i64* inOutTStates);

// Load a buffer into memory, ignoring write-only state of ROMs
void memoryLoad(Memory* mem, u16 address, void* buffer, u16 size);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <memory.h>
#include <kore/k_random.h>

#define LO(x) ((u8)((x) % 256))
#define HI(x) ((u8)((x) / 256))

//----------------------------------------------------------------------------------------------------------------------
// Management
//----------------------------------------------------------------------------------------------------------------------

bool memoryOpen(Memory* mem)
{
    mem->memory = K_ALLOC(KB(64));
    if (!mem->memory) return NO;

    // Build contention table
    mem->contention = K_ALLOC_CLEAR(sizeof(*mem->contention) * 70930);
    int contentionStart = 14335;
    int contentionEnd = contentionStart + (192 * 224);
    int t = contentionStart;

    while (t < contentionEnd)
    {
        // Calculate contention for the next 128 t-states (i.e. a single pixel line)
        for (int i = 0; i < 128; i += 8)
        {
            mem->contention[t++] = 6;
            mem->contention[t++] = 5;
            mem->contention[t++] = 4;
            mem->contention[t++] = 3;
            mem->contention[t++] = 2;
            mem->contention[t++] = 1;
            mem->contention[t++] = 0;
            mem->contention[t++] = 0;
        }

        // Skip the time the border is being drawn: 96 t-states (24 left, 24 right, 48 retrace)
        t += (224 - 128);
    }

    // Fill memory up with random stuff
    Random r;
    randomInit(&r);
    for (u16 a = 0; a < 0xffff; ++a)
    {
        i64 ts = 0;
        memoryPoke(mem, a, (u8)random64(&r), &ts);
    }

    return YES;
}

void memoryClose(Memory* mem)
{
    K_FREE(mem->contention, sizeof(*mem->contention) * 70930);
    K_FREE(mem->memory, KB(64));
}

//----------------------------------------------------------------------------------------------------------------------
// Attributes
//----------------------------------------------------------------------------------------------------------------------

i64 memoryContention(Memory* mem, u16 addr, i64 tStates)
{
    // Check for slot 2: Address = SS00 0000 0000 0000, where SS = 01
    if ((addr & 0xc000) == 0x4000)
    {
        K_ASSERT(tStates < 70930);
        return mem->contention[tStates];
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
// Setting bytes
//----------------------------------------------------------------------------------------------------------------------

void memoryPoke(Memory* mem, u16 address, u8 b, i64* inOutTStates)
{
    *inOutTStates += memoryContention(mem, address, *inOutTStates);
    *inOutTStates += 3;
    if (address >= 0x4000) mem->memory[address] = b;
}

void memoryPoke16(Memory* mem, u16 address, u16 w, i64* inOutTStates)
{
    memoryPoke(mem, address, LO(w), inOutTStates);
    memoryPoke(mem, address + 1, HI(w), inOutTStates);
}

u8 memoryPeek(Memory* mem, u16 address, i64* inOutTStates)
{
    *inOutTStates += memoryContention(mem, address, *inOutTStates);
    *inOutTStates += 3;
    return mem->memory[address];
}

u16 memoryPeek16(Memory* mem, u16 address, i64* inOutTStates)
{
    return memoryPeek(mem, address, inOutTStates) + 256 * memoryPeek(mem, address + 1, inOutTStates);
}

Ref memoryRef(Memory* mem, u16 address, i64* inOutTStates)
{
    static u8 dump;
    Ref r = { &mem->memory[address], &mem->memory[address] };
    if (address < 0x4000)
    {
        r.w = &dump;
    }

    *inOutTStates += memoryContention(mem, address, *inOutTStates);

    return r;
}

void memoryLoad(Memory* mem, u16 address, void* buffer, u16 size)
{
    i64 clampedSize = K_MIN((i64)address + (i64)size, 65536) - address;
    memoryCopy(buffer, &mem->memory[address], clampedSize);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
