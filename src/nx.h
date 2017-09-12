//----------------------------------------------------------------------------------------------------------------------
// NX system
// Launches other subsystems based command line arguments, and manages memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "machine.h"

//----------------------------------------------------------------------------------------------------------------------
// Callback types
//----------------------------------------------------------------------------------------------------------------------

// Manages all memory operations.  The table below demonstrates how to use this:
//
//  Operation   oldAddress      newSize
//  ----------+---------------+------------------
//  ALLOC     | 0             | allocation size
//  REALLOC   | address       | new size
//  FREE      | address       | 0
//
typedef void* (* NxMemoryOperationFunc) (void* oldAddress, i64 newSize);

typedef struct
{
    int                     argc;
    char**                  argv;
    NxMemoryOperationFunc   memFunc;        // Callback to handle all operations
    MachineConfig           machineConfig;
}
NxConfig;

typedef struct _Nx* Nx;

//----------------------------------------------------------------------------------------------------------------------
// API to platform layer
//----------------------------------------------------------------------------------------------------------------------

// Initialise the NX system by providing callback functions for platform-specific memory and IO operations.
bool nxOpen(Nx N, NxConfig* config);

// Close down the NX system
void nxClose(Nx N);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

//----------------------------------------------------------------------------------------------------------------------
// Global structure
//----------------------------------------------------------------------------------------------------------------------

struct _Nx
{
    NxConfig    config;
    Machine     machine;
};

//----------------------------------------------------------------------------------------------------------------------
// Memory operations
//----------------------------------------------------------------------------------------------------------------------

static void* nxAlloc(Nx N, i64 numBytes)
{
    ASSERT(N && N->config.memFunc);
    return N->config.memFunc(0, numBytes);
}

static void* nxRealloc(Nx N, void* address, i64 newNumBytes)
{
    ASSERT(N && N->config.memFunc);
    return N->config.memFunc(address, newNumBytes);
}

static void nxFree(Nx N, void* address)
{
    ASSERT(N && N->config.memFunc);
    N->config.memFunc(address, 0);
}

//----------------------------------------------------------------------------------------------------------------------
// Public API
//----------------------------------------------------------------------------------------------------------------------

bool nxOpen(Nx N, NxConfig* config)
{
    ASSERT(N);
    ASSERT(config);

    N->config = *config;
    return machineOpen(&N->machine, &config->machineConfig);
}

void nxClose(Nx N)
{
    ASSERT(N);
    machineClose(&N->machine);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
