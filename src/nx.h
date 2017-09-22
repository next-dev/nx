//----------------------------------------------------------------------------------------------------------------------
// NX system
// Launches other subsystems based command line arguments, and manages memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "config.h"
#include "machine.h"

typedef struct
{
    i64         tState;
    Machine     machine;
}
Nx;

// Output structure returned by nxUpdate to communicate with the platform-specific layer.
typedef struct 
{
    i64     startTState;        // Starting t-state when nxUpdate is called
    i64     endTState;          // Ending t-state when nxUpdate is finished

    // Signals
    bool    redraw;
}
NxOut;

//----------------------------------------------------------------------------------------------------------------------
// API to platform layer
//----------------------------------------------------------------------------------------------------------------------

// Initialise the NX system.  The image array past must be of size NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT as defined in
// video.h.  The format should be BGRA BGRA... (or ARGB in each u32).
bool nxOpen(Nx* N, u32* img);

// Close down the NX system
void nxClose(Nx* N);

// Advance as many number of t-states as possible.  Returns a NxOut structure.  This will contain information about
// the new tState.  The client should calculate the time to wait for before calling nxUpdate again.  The NxOut
// structure will also contain booleans to signal to the client what to do.
//
//      redraw = YES        Redraw window.
//
NxOut nxUpdate(Nx* N);

//----------------------------------------------------------------------------------------------------------------------
// Event system
//----------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include "test.h"

//----------------------------------------------------------------------------------------------------------------------
// Public API
//----------------------------------------------------------------------------------------------------------------------

bool nxOpen(Nx* N, u32* img)
{
    K_ASSERT(N);

#if NX_RUN_TESTS

    Tests T;
    if (testOpen(&T))
    {
        int count = testCount(&T);
        for (int i = 0; i < count; ++i)
        {
            if (!testRun(&T, i)) K_BREAK();
        }
        testClose(&T);
    }

#endif // NX_RUN_TESTS

    N->tState = 0;
    return machineOpen(&N->machine, img);
}

void nxClose(Nx* N)
{
    K_ASSERT(N);
    machineClose(&N->machine);
}

NxOut nxUpdate(Nx* N)
{
    NxOut out;

    out.startTState = N->tState;
    out.redraw = NO;

    for(;;)
    {
        i64 newTState = machineUpdate(&N->machine, N->tState);
        if (newTState >= 69888)
        {
            out.endTState = newTState;
            out.redraw = YES;
            N->tState = newTState - 69888;
            break;
        }
        else
        {
            N->tState = newTState;
        }
    }

    return out;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
