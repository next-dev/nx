//----------------------------------------------------------------------------------------------------------------------
// Common data structure for all machines
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "memory.h"
#include "video.h"

typedef struct  
{
    Memory      memory;
    Video       video;
    u8          border;     // Write to port $fe will change this.
}
Machine;

// Create a ZX Spectrum 48K machine.  The image passed will be rendered too from time to time.
bool machineOpen(Machine* M, u32* img);

// Destroy a machine.
void machineClose(Machine* M);

// Run a few t-states.  Returns the new tstate position
i64 machineUpdate(Machine* M, i64 tState);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <kore/k_blob.h>

bool machineOpen(Machine* M, u32* img)
{
    // Initialise sub-systems
    if (!memoryOpen(&M->memory)) goto error;
    if (!videoOpen(&M->video, &M->memory, &M->border, img)) goto error;

    // Initialise state
    M->border = 7;

    // Load the ROM into memory.
    Blob rom = blobLoad("48.rom");
    if (rom.bytes)
    {
        memoryLoad(&M->memory, 0, rom.bytes, (u16)rom.size);
        blobUnload(rom);
    }

    return YES;

error:
    machineClose(M);
    return NO;
}

void machineClose(Machine* M)
{
    videoClose(&M->video);
    memoryClose(&M->memory);
}

i64 machineUpdate(Machine* M, i64 tState)
{
    // # t-states per frame is (64+192+56)*(48+128+48) = 69888
    if ((tState % 69888) == 0)
    {
        videoRenderULA(&M->video);
    }
    return ++tState;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL