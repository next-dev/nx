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
}
Memory;

typedef struct 
{
    u8*         r;      // Read reference
    u8*         w;      // Write reference
}
Ref;

// Initialise the memory
bool memoryOpen(Memory* mem);

// Close the memory sub-system
void memoryClose(Memory* mem);

// Set an 8-bit value to the memory.  Will not write if the memory address points to ROM.
void memorySet(Memory* mem, u16 address, u8 b);

// Set a 16-bit value to the memory in little endian form.
void memorySet16(Memory* mem, u16 address, u16 w);

// Read an 8-bit value from the memory.
u8 memoryGet(Memory* mem, u16 address);

// Read an 16-bit value from the memory.
u16 memoryGet16(Memory* mem, u16 address);

// Get a reference to a memory address.
Ref memoryRef(Memory* mem, u16 address);

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

    return YES;
}

void memoryClose(Memory* mem)
{
    K_FREE(mem->memory, KB(64));
}

//----------------------------------------------------------------------------------------------------------------------
// Setting bytes
//----------------------------------------------------------------------------------------------------------------------

void memorySet(Memory* mem, u16 address, u8 b)
{
    if (address >= 0x4000) mem->memory[address] = b;
}

void memorySet16(Memory* mem, u16 address, u16 w)
{
    memorySet(mem, address, LO(w));
    memorySet(mem, address + 1, HI(w));
}

u8 memoryGet(Memory* mem, u16 address)
{
    return mem->memory[address];
}

u16 memoryGet16(Memory* mem, u16 address)
{
    return memoryGet(mem, address) + 256 * memoryGet(mem, address + 1);
}

Ref memoryRef(Memory* mem, u16 address)
{
    static u8 dump;
    Ref r = { &mem->memory[address], &mem->memory[address] };
    if (address < 0x4000)
    {
        r.w = &dump;
    }

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
