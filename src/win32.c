//----------------------------------------------------------------------------------------------------------------------
// Win32 wrapper for NX Emulator
//----------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//----------------------------------------------------------------------------------------------------------------------
// Platform/compiler specific basic types and definitions.
// To port need to define:
//
//      - i8 i16 i32 i64
//      - u8 u16 u32 u64
//      - f32 f64
//      - bool
//      - YES NO AS_BOOL
//      - ASSERT
//
// These need to be defined before including any other header libraries in this package.
//
//----------------------------------------------------------------------------------------------------------------------

typedef INT8        i8;
typedef INT16       i16;
typedef INT32       i32;
typedef INT64       i64;

typedef UINT8       u8;
typedef UINT16      u16;
typedef UINT32      u32;
typedef UINT64      u64;

typedef float       f32;
typedef double      f64;

typedef int         bool;

#define YES (1)
#define NO (0)
#define AS_BOOL(b) ((b) ? YES : NO)

#include <assert.h>
#define ASSERT(x, ...) assert(x)

//----------------------------------------------------------------------------------------------------------------------
// Include the header libraries
//----------------------------------------------------------------------------------------------------------------------

#define NX_IMPL
#include "nx.h"

//----------------------------------------------------------------------------------------------------------------------
// Win32 callbacks
//----------------------------------------------------------------------------------------------------------------------

#include <stdlib.h>

void* MemoryOperation(void* address, i64 numBytes)
{
    void* p = 0;

    if (0 == numBytes)
    {
        free(address);
    }
    else
    {
        p = realloc(address, numBytes);
    }

    return p;
}

//----------------------------------------------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdLine, int showCmd)
{
    // Step 1: Initialise the NX system with the platform specific callbacks.
    NxConfig config = {
        __argc,                 // argc
        __argv,                 // argv
        &MemoryOperation,       // memFunc
        { 0 },                  // MachineConfig
    };

    struct _Nx N;

    if (nxOpen(&N, &config))
    {
        nxClose(&N);
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
