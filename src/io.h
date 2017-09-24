//----------------------------------------------------------------------------------------------------------------------
// Emulates the IO ports and peripherals
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/k_platform.h>
#include "memory.h"

typedef struct  
{
    u8          border;     // The border colour.
    Memory*     memory;     // Access to the contention tables
}
Io;


// Initialise the I/O & peripherals
void ioInit(Io* io, Memory* memory);

// Contends the machine for a given port for t t-states, for n times.
void ioContend(Io* io, u16 port, i64 tStates, int num, i64* inOutTStates);

// Write out a byte to an I/O port.
void ioOut(Io* io, u16 port, u8 data, i64* inOutTStates);

// Read in a byte from an I/O port.
u8 ioIn(Io* io, u16 port, i64* inOutTStates);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include "test.h"
#include "memory.h"

void ioInit(Io* io, Memory* memory)
{
    io->border = 7;
    io->memory = memory;
}

void ioContend(Io* io, u16 port, i64 tStates, int num, i64* inOutTStates)
{
#if NX_RUN_TESTS
    if (gResultsFile)
    {
        i64 ts = *inOutTStates;
        for (int i = 0; i < num; ++i)
        {
            fprintf(gResultsFile, "%5d PC %04x\n", (int)ts, port);
            ts += tStates;
        }
    }
#endif

    if (memoryIsContended(io->memory, port))
    {
        for (int i = 0; i < num; ++i)
        {
            *inOutTStates += io->memory->contention[*inOutTStates] + tStates;
        }
    }
    else
    {
        *inOutTStates += tStates * num;
    }
}

void ioOut(Io* io, u16 port, u8 data, i64* inOutTStates)
{
    if (memoryIsContended(io->memory, port))
    {
        ioContend(io, port, 1, 1, inOutTStates);
    }
    else
    {
        ++(*inOutTStates);
    }

#if NX_RUN_TESTS
    if (gResultsFile) fprintf(gResultsFile, "%5d PW %04x %02x\n", (int)*inOutTStates, port, data);
#endif

    bool ulaPort = K_BOOL((port & 1) == 0);
    if (ulaPort)
    {
        // Deal with out $fe.
    }

    if (ulaPort)
    {
        ioContend(io, port, 3, 1, inOutTStates);
    }
    else
    {
        if (memoryIsContended(io->memory, port))
        {
            ioContend(io, port, 1, 3, inOutTStates);
        }
        else
        {
            *inOutTStates += 3;
        }
    }
}

u8 ioIn(Io* io, u16 port, i64* inOutTStates)
{
    return 0xff;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
