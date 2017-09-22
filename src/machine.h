//----------------------------------------------------------------------------------------------------------------------
// Common data structure for all machines
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "memory.h"
#include "video.h"

typedef struct _Machine Machine;

// This is the signature of an event handler.  The handler can change the number of t-states.  Return YES if you
// want machineUpdate to continue.  If the output tState count is less than the original, all events trigger points
// will be decreased by the same amount.
typedef bool(*EventFunc) (Machine* M, i64* inOutTState);

typedef struct  
{
    i64         tState;
    EventFunc   eventFunc;
}
Event;

struct _Machine
{
    Memory          memory;
    Video           video;
    int             frameCounter;   // incremented each frame to time flash effect
    u8              border;         // Write to port $fe will change this.
    Array(Event)    events;
};

// Create a ZX Spectrum 48K machine.  The image passed will be rendered too from time to time.
bool machineOpen(Machine* M, u32* img);

// Destroy a machine.
void machineClose(Machine* M);

// Run a few t-states.  Returns the new t-state position
i64 machineUpdate(Machine* M, i64 tState);

// Events - events call functions when the tstate counter equals or passes a given value.  As soon as an event
// occurs machineUpdate will return if the event function returns NO.

// Add a new event
void machineAddEvent(Machine* M, i64 tState, EventFunc eventFunc);

// Test for an event for a given t-state.  If there is no event, YES is returned.  Otherwise the event handler
// is called and the result of that handler is returned.
bool machineTestEvent(Machine* M, i64* inOutTState);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <kore/k_blob.h>
#include "z80.h"

#define MACHINE_EVENT(name) internal bool name(Machine* M, i64* inOutTState)

bool machineOpen(Machine* M, u32* img)
{
    // Initialise sub-systems
    if (!memoryOpen(&M->memory)) goto error;
    if (!videoOpen(&M->video, &M->memory, &M->border, img)) goto error;

    // Initialise state
    M->border = 7;
    M->events = 0;

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

MACHINE_EVENT(machineInterrupt)
{
    //*inOutTState -= 69888;
    videoRenderULA(&M->video, K_BOOL(M->frameCounter++ & 16));
    return NO;
}

i64 machineUpdate(Machine* M, i64 tState)
{
    machineAddEvent(M, 69888, &machineInterrupt);
    while (machineTestEvent(M, &tState))
    {
        ++tState;
    }

    // # t-states per frame is (64+192+56)*(48+128+48) = 69888
//    if (tState >= 69888)
//    {
//         static TimePoint t = { 0 };
//         static bool flash = NO;
// 
//         if (!flash && (M->frameCounter & 16))
//         {
//             flash = YES;
//             TimePoint tt = now();
//             TimePeriod dt = period(t, tt);
//             printf("FLASH TIME: %dms\n", (int)toMSecs(dt));
//             t = tt;
//         }
//         else if ((M->frameCounter & 16) == 0)
//         {
//             flash = NO;
//         }

//        videoRenderULA(&M->video, K_BOOL(M->frameCounter++ & 16));
//    }

    return tState;
}

void machineAddEvent(Machine* M, i64 tState, EventFunc eventFunc)
{
    Event* e = arrayExpand(M->events, 1);
    e->tState = tState;
    e->eventFunc = eventFunc;

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
            if (!M->events[0].eventFunc(M, inOutTState)) result = NO;
            if (*inOutTState < tState)
            {
                // We've gone back in time.  Process the following events
                i64 diff = *inOutTState - tState;
                for (int i = 1; i < arrayCount(M->events); ++i)
                {
                    M->events[i].tState += diff;
                }
            }

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
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
