//----------------------------------------------------------------------------------------------------------------------
// Win32 wrapper for NX Emulator
//----------------------------------------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define K_IMPLEMENTATION
#include <kore/k_memory.h>
#include <kore/k_window.h>

#define NX_MINOR_VERSION        0
#define NX_MAJOR_VERSION        0
#define NX_PATCH_VERSION        1

#define NX_STR2(x) #x
#define NX_STR(x) NX_STR2(x)

#if NX_MAJOR_VERSION == 0
#   if NX_MINOR_VERSION == 0
#       define NX_VERSION "Dev." NX_STR(NX_PATCH_VERSION)
#   elif NX_MINOR_VERSION == 9
#       define NX_VERSION "Beta." NX_STR(NX_PATCH_VERSION)
#   else
#       define NX_VERSION "Alpha." NX_STR(NX_PATCH_VERSION)
#   endif
#else
#   define NX_VERSION NX_STR(NX_MAJOR_VERSION) "." NX_STR(NX_MINOR_VERSION) "." NX_STR(NX_PATCH_VERSION)
#endif

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
    debugBreakOnAlloc(0);

    // Step 1: Initialise the NX system with the platform specific callbacks.
    Nx N;
    u32* img = K_ALLOC(NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT * sizeof(u32));

    if (nxOpen(&N, img))
    {
        Window w = windowMake("NX (" NX_VERSION ")", img, NX_WINDOW_WIDTH, NX_WINDOW_HEIGHT, 3);
        //windowConsole();
        TimePoint t = { 0 };

        LARGE_INTEGER qpf;
        QueryPerformanceFrequency(&qpf);

        while (windowPump())
        {
            t = now();
            NxOut out = nxUpdate(&N);
            if (out.redraw)
            {
                windowRedraw(w);
            }

            i64 msElapsed = out.elapsedTStates * 20 / 69888;
            TimePoint futureTime = future(t, msecs(msElapsed));
            waitUntil(futureTime);
        }

        nxClose(&N);
    }

    K_FREE(img, NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT * sizeof(u32));

#if NX_RUN_TESTS
    windowConsolePause();
#endif

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
