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
    int key1 = K_COUNT;
    int key2 = K_COUNT;

    switch (vkCode)
    {
    case '1':           key1 = K_1;         break;
    case '2':           key1 = K_2;         break;
    case '3':           key1 = K_3;         break;
    case '4':           key1 = K_4;         break;
    case '5':           key1 = K_5;         break;
    case '6':           key1 = K_6;         break;
    case '7':           key1 = K_7;         break;
    case '8':           key1 = K_8;         break;
    case '9':           key1 = K_9;         break;
    case '0':           key1 = K_0;         break;

    case 'A':           key1 = K_A;         break;
    case 'B':           key1 = K_B;         break;
    case 'C':           key1 = K_C;         break;
    case 'D':           key1 = K_D;         break;
    case 'E':           key1 = K_E;         break;
    case 'F':           key1 = K_F;         break;
    case 'G':           key1 = K_G;         break;
    case 'H':           key1 = K_H;         break;
    case 'I':           key1 = K_I;         break;
    case 'J':           key1 = K_J;         break;
    case 'K':           key1 = K_K;         break;
    case 'L':           key1 = K_L;         break;
    case 'M':           key1 = K_M;         break;
    case 'N':           key1 = K_N;         break;
    case 'O':           key1 = K_O;         break;
    case 'P':           key1 = K_P;         break;
    case 'Q':           key1 = K_Q;         break;
    case 'R':           key1 = K_R;         break;
    case 'S':           key1 = K_S;         break;
    case 'T':           key1 = K_T;         break;
    case 'U':           key1 = K_U;         break;
    case 'V':           key1 = K_V;         break;
    case 'W':           key1 = K_W;         break;
    case 'X':           key1 = K_X;         break;
    case 'Y':           key1 = K_Y;         break;
    case 'Z':           key1 = K_Z;         break;

    case VK_SHIFT:      key1 = K_Shift;     break;
    case VK_CONTROL:    key1 = K_SymShift;  break;
    case VK_RETURN:     key1 = K_Enter;     break;
    case VK_SPACE:      key1 = K_Space;     break;

    case VK_BACK:       key1 = K_Shift;     key2 = K_0;         break;
    case VK_ESCAPE:     key1 = K_Shift;     key2 = K_Space;     break;
    case VK_LEFT:       key1 = K_Shift;     key2 = K_5;         break;
    case VK_DOWN:       key1 = K_Shift;     key2 = K_6;         break;
    case VK_UP:         key1 = K_Shift;     key2 = K_7;         break;
    case VK_RIGHT:      key1 = K_Shift;     key2 = K_8;         break;
    }

    if (key1 < K_COUNT)
    {
        N->keys[key1] = down ? 1 : 0;
    }
    if (key2 < K_COUNT)
    {
        N->keys[key2] = down ? 1 : 0;
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
