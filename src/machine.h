//----------------------------------------------------------------------------------------------------------------------
// Common data structure for all machines
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "memory.h"
#include "video.h"
#include "z80.h"
#include "io.h"

typedef struct _Machine Machine;

// This is the signature of an event handler.  The handler can change the number of t-states.  Return YES if you
// want machineUpdate to continue.  If the output tState count is less than the original, all events trigger points
// will be decreased by the same amount.
typedef bool(*EventFunc) (Machine* M, i64* inOutTState);

typedef struct  
{
    i64         tState;
    EventFunc   eventFunc;
    const char* name;
}
Event;

typedef struct
{
    u32*    img;        // The image to render to.
    u8*     keys;       // The key state.
}
MachineConfig;

struct _Machine
{
    Memory          memory;
    Video           video;
    int             frameCounter;   // incremented each frame to time flash effect
    Array(Event)    events;
    Z80             z80;            // The cpu emulator
    Io              io;

    // Output - when the machine has stopped updating, these will reflect events that the emulator should react to
    bool            redraw;
};

// Create a ZX Spectrum 48K machine.  The image passed will be rendered too from time to time.
bool machineOpen(Machine* M, MachineConfig* config);

// Destroy a machine.
void machineClose(Machine* M);

// Run a few t-states.  Returns the new t-state position
i64 machineUpdate(Machine* M, i64* tState);

// Events - events call functions when the tstate counter equals or passes a given value.  As soon as an event
// occurs machineUpdate will return if the event function returns NO.

// Add a new event
void machineAddEvent(Machine* M, i64 tState, EventFunc eventFunc, const char* name);

// Test for an event for a given t-state.  If there is no event, YES is returned.  Otherwise the event handler
// is called and the result of that handler is returned.
bool machineTestEvent(Machine* M, i64* inOutTState);

// Macro to define machine event handlers.
#define MACHINE_EVENT(name) internal bool name(Machine* M, i64* inOutTState)

// Load in a file
typedef enum
{
    FT_Sna,
    FT_Tap,
    FT_Psz,
}
FileType;

bool machineLoad(Machine* M, const u8* data, i64 size, FileType typeHint, i64* outTState);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <kore/k_blob.h>

MACHINE_EVENT(machineFrame)
{
    machineAddEvent(M, (69888 * NX_SPEED), &machineFrame, "frame");
    *inOutTState -= (69888 * NX_SPEED);
    videoRenderULA(&M->video, K_BOOL(M->frameCounter++ & 16));
    M->redraw = YES;
    z80Interrupt(&M->z80);
    return NO;
}

bool machineOpen(Machine* M, MachineConfig* config)
{
    // Initialise sub-systems
    if (!memoryOpen(&M->memory)) goto error;
    ioInit(&M->io, &M->memory, config->keys);
    if (!videoOpen(&M->video, &M->memory, &M->io.border, config->img)) goto error;
    z80Init(&M->z80, &M->memory, &M->io);

    // Initialise state
    M->events = 0;

    // Load the ROM into memory.
    Blob rom = blobLoad("48.rom");
    if (rom.bytes)
    {
        memoryLoad(&M->memory, 0, rom.bytes, (u16)rom.size);
        blobUnload(rom);
    }

    // Initial events
    machineAddEvent(M, (69888 * NX_SPEED), &machineFrame, "frame");

    return YES;

error:
    machineClose(M);
    return NO;
}

void machineClose(Machine* M)
{
    videoClose(&M->video);
    memoryClose(&M->memory);
    arrayRelease(M->events);
}

i64 machineUpdate(Machine* M, i64* tState)
{
    i64 elapsedTStates = 0;
    M->redraw = NO;

    while (machineTestEvent(M, tState))
    {
        i64 startTState = *tState;
        z80Step(&M->z80, tState);
        elapsedTStates += (*tState - startTState);
    }

    return elapsedTStates;
}

void machineAddEvent(Machine* M, i64 tState, EventFunc eventFunc, const char* name)
{
#if NX_DEBUG_EVENTS
    printf("Add Event: (%d) %s\n", (int)tState, name);
#endif

    Event* e = arrayExpand(M->events, 1);
    e->tState = tState;
    e->eventFunc = eventFunc;
    e->name = name;

    // Sort the array
    i64 size = arrayCount(M->events);
    if (size > 1)
    {
        for (i64 i = size - 1; i > 0; --i)
        {
            Event* e1 = &M->events[i - 1];
            Event* e2 = &M->events[i];

            // Check to see if the element is in the right place.
            if (e1->tState < e2->tState) break;

            // No!  Swap and continue.
            Event t = *e1;
            *e1 = *e2;
            *e2 = t;
        }
    }
}

bool machineTestEvent(Machine* M, i64* inOutTState)
{
    bool result = YES;

    // Keep looping until all events have been processed
    while(arrayCount(M->events) > 0)
    {
        i64 tState = *inOutTState;
        if (M->events[0].tState <= tState)
        {
            // We need to run this event.
#if NX_DEBUG_EVENTS
            printf("\033[33;1mRun Event: (%d) %s\033[0m\n", (int)tState, M->events[0].name);
#endif

            if (!M->events[0].eventFunc(M, inOutTState)) result = NO;

            // Remove this event
            arrayDelete(M->events, 0);
        }
        else
        {
            // No events just yet.
            break;
        }
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------
// File loading
//----------------------------------------------------------------------------------------------------------------------

bool machineLoad(Machine* M, const u8* data, i64 size, FileType typeHint, i64* outTState)
{
    return NO;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
