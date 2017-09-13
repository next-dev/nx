//----------------------------------------------------------------------------------------------------------------------
// Win32 wrapper for NX Emulator
//----------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define K_IMPLEMENTATION
#include <kore/k_memory.h>
#include <kore/k_window.h>

//----------------------------------------------------------------------------------------------------------------------
// Include the header libraries
//----------------------------------------------------------------------------------------------------------------------

#define NX_IMPL
#include "nx.h"

//----------------------------------------------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------------------------------------------

int kmain(int argc, char** argv)
{
    // Step 1: Initialise the NX system with the platform specific callbacks.
    Nx N;
    u32* img = K_ALLOC(NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT * sizeof(u32));

    if (nxOpen(&N, img))
    {
        Window w = windowMake("NX 0.1", img, NX_WINDOW_WIDTH, NX_WINDOW_HEIGHT, 3);
        i64 tState = 0;

        while (windowPump())
        {
            tState = nxUpdate(&N, tState);
            if (tState > 69888)
            {
                tState -= 69888;
                windowRedraw(w);
            }
        }

        windowClose(w);

        nxClose(&N);
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
