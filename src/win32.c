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

void key(Nx* N, u8 vkCode, bool down)
{
    int key = K_COUNT;

    switch (vkCode)
    {
    case '1':           key = K_1;          break;
    case '2':           key = K_2;          break;
    case '3':           key = K_3;          break;
    case '4':           key = K_4;          break;
    case '5':           key = K_5;          break;
    case '6':           key = K_6;          break;
    case '7':           key = K_7;          break;
    case '8':           key = K_8;          break;
    case '9':           key = K_9;          break;
    case '0':           key = K_0;          break;

    case 'A':           key = K_A;          break;
    case 'B':           key = K_B;          break;
    case 'C':           key = K_C;          break;
    case 'D':           key = K_D;          break;
    case 'E':           key = K_E;          break;
    case 'F':           key = K_F;          break;
    case 'G':           key = K_G;          break;
    case 'H':           key = K_H;          break;
    case 'I':           key = K_I;          break;
    case 'J':           key = K_J;          break;
    case 'K':           key = K_K;          break;
    case 'L':           key = K_L;          break;
    case 'M':           key = K_M;          break;
    case 'N':           key = K_N;          break;
    case 'O':           key = K_O;          break;
    case 'P':           key = K_P;          break;
    case 'Q':           key = K_Q;          break;
    case 'R':           key = K_R;          break;
    case 'S':           key = K_S;          break;
    case 'T':           key = K_T;          break;
    case 'U':           key = K_U;          break;
    case 'V':           key = K_V;          break;
    case 'W':           key = K_W;          break;
    case 'X':           key = K_X;          break;
    case 'Y':           key = K_Y;          break;
    case 'Z':           key = K_Z;          break;

    case VK_SHIFT:      key = K_Shift;      break;
    case VK_CONTROL:    key = K_SymShift;   break;
    case VK_RETURN:     key = K_Enter;      break;
    case VK_SPACE:      key = K_Space;      break;
    }

    if (key < K_COUNT)
    {
        N->keys[key] = down;
    }
}

void keyDown(Window wnd, u8 vkCode, void* data)
{
    Nx* N = (Nx *)data;
    key(N, vkCode, YES);
}

void keyUp(Window wnd, u8 vkCode, void* data)
{
    Nx* N = (Nx *)data;
    key(N, vkCode, NO);
}

int kmain(int argc, char** argv)
{
    debugBreakOnAlloc(0);

#if NX_DEBUG_EVENTS || NX_DEBUG_HARDWARE
    windowConsole();
#endif

    // Step 1: Initialise the NX system with the platform specific callbacks.
    Nx N;
    u32* img = K_ALLOC(NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT * sizeof(u32));

    if (nxOpen(&N, img))
    {
        Window w = windowMake("NX (" NX_VERSION ")", img, NX_WINDOW_WIDTH, NX_WINDOW_HEIGHT, 3, &N);
        windowHandleKeyDownEvent(w, &keyDown);
        windowHandleKeyUpEvent(w, &keyUp);
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
