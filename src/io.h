//----------------------------------------------------------------------------------------------------------------------
// Emulates the IO ports and peripherals
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/k_platform.h>
#include "memory.h"

typedef enum
{
    // Keys related to Spectrum keyboard
    K_Shift, K_Z, K_X, K_C, K_V,
    K_A, K_S, K_D, K_F, K_G,
    K_Q, K_W, K_E, K_R, K_T,
    K_1, K_2, K_3, K_4, K_5,
    K_0, K_9, K_8, K_7, K_6,
    K_P, K_O, K_I, K_U, K_Y,
    K_Enter, K_L, K_K, K_J, K_H,
    K_Space, K_SymShift, K_M, K_N, K_B,

    K_COUNT
}
Keys;

typedef struct  
{
    u8          border;     // The border colour.
    Memory*     memory;     // Access to the contention tables
    u8*         keys;       // Current key state
}
Io;


// Initialise the I/O & peripherals
void ioInit(Io* io, Memory* memory, u8* keys);

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

void ioInit(Io* io, Memory* memory, u8* keys)
{
    io->border = 7;
    io->memory = memory;
    io->keys = keys;
    memoryClear(io->keys, K_COUNT);
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

    //
    // Send the value to the port
    //

    bool ulaPort = K_BOOL((port & 1) == 0);
    if (ulaPort)
    {
        // Deal with out $fe.
        io->border = data & 7;
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
    u8 x = 0;
    bool ulaPort = K_BOOL((port & 1) == 0);

    if (memoryIsContended(io->memory, port))
    {
        ioContend(io, port, 1, 1, inOutTStates);
    }
    else
    {
        ++(*inOutTStates);
    }

#if NX_RUN_TESTS
    x = HI(port);
    if (gResultsFile) fprintf(gResultsFile, "%5d PR %04x %02x\n", (int)*inOutTStates, port, x);
#endif

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

    //
    // Fetch the actual value from the port
    //

    if (ulaPort)
    {
#if NX_DEBUG_HARDWARE
        static u8 oldKeys[K_COUNT] = { 0 };
        static const char* keyNames[K_COUNT] = {
            "Shift", "Z", "X", "C", "V",
            "A", "S", "D", "F", "G",
            "Q", "W", "E", "R", "T",
            "1", "2", "3", "4", "5",
            "0", "9", "8", "7", "6",
            "P", "O", "I", "U", "Y",
            "Enter", "L", "K", "J", "H",
            "Space", "SymShift", "M", "N", "B"
        };
        for (int i = 0; i < K_COUNT; ++i)
        {
            if (io->keys[i] != oldKeys[i])
            {
                printf("KEY: \033[3%c;1m[%s]\033[0m\n", io->keys[i] ? '2' : '1', keyNames[i]);
            }
        }
        memoryCopy(io->keys, oldKeys, K_COUNT * sizeof(oldKeys[0]));
#endif
        // Deal with in $fe.
        if (LO(port) == 0xfe)
        {
            x = 0xff;
            u8 row = HI(port);
            for (int i = 0; i < 8; ++i)
            {
                if ((row & 1) == 0)
                {
                    // This row is required to be read
                    u8 bit = 1;
                    u8 keys = 0;
                    for (int j = 0; j < 5; ++j)
                    {
                        if (io->keys[i * 5 + j])
                        {
                            // Key is down!
                            keys |= bit;
                        }
                        bit <<= 1;
                    }
                    x &= ~keys;
                }
                row >>= 1;
            }
        }
    }

    return x;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
