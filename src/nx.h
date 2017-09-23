//----------------------------------------------------------------------------------------------------------------------
// NX system
// Launches other subsystems based command line arguments, and manages memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "config.h"
#include "machine.h"

typedef struct
{
    i64         tState;
    Machine     machine;
}
Nx;

// Output structure returned by nxUpdate to communicate with the platform-specific layer.
typedef struct 
{
    i64     elapsedTStates;     // T-states emulated in update

    // Signals
    bool    redraw;
}
NxOut;

//----------------------------------------------------------------------------------------------------------------------
// API to platform layer
//----------------------------------------------------------------------------------------------------------------------

// Initialise the NX system.  The image array past must be of size NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT as defined in
// video.h.  The format should be BGRA BGRA... (or ARGB in each u32).
bool nxOpen(Nx* N, u32* img);

// Close down the NX system
void nxClose(Nx* N);

// Advance as many number of t-states as possible.  Returns a NxOut structure.  This will contain information about
// the new tState.  The client should calculate the time to wait for before calling nxUpdate again.  The NxOut
// structure will also contain booleans to signal to the client what to do.
//
//      redraw = YES        Redraw window.
//
NxOut nxUpdate(Nx* N);

//----------------------------------------------------------------------------------------------------------------------
// Test system
//----------------------------------------------------------------------------------------------------------------------

#if NX_RUN_TESTS
// Run a test
bool testRun(Tests* T, int index);
#endif

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include "test.h"

//----------------------------------------------------------------------------------------------------------------------
// Test system
//----------------------------------------------------------------------------------------------------------------------

#if NX_RUN_TESTS

MACHINE_EVENT(testEnd)
{
    return NO;
}

bool testRun(Tests* T, int index)
{
    TestIn* testIn = &T->tests[index];
    //printf("TEST: %s\n", testIn->name);
    fprintf(gResultsFile, "%s\n", testIn->name);

    // Create a machine with no display.
    Machine M;
    i64 tStates = 0;
    if (machineOpen(&M, 0))
    {
        memoryReset(&M.memory);
        machineAddEvent(&M, testIn->tStates, &testEnd);

        // Set up the registers
        M.z80.af.r = testIn->af;
        M.z80.bc.r = testIn->bc;
        M.z80.de.r = testIn->de;
        M.z80.hl.r = testIn->hl;
        M.z80.af_.r = testIn->af_;
        M.z80.bc_.r = testIn->bc_;
        M.z80.de_.r = testIn->de_;
        M.z80.hl_.r = testIn->hl_;
        M.z80.ix.r = testIn->ix;
        M.z80.iy.r = testIn->iy;
        M.z80.sp.r = testIn->sp;
        M.z80.pc.r = testIn->pc;
        M.z80.m.r = testIn->mp;
        M.z80.ir.h = testIn->i;
        M.z80.ir.l = testIn->r;
        M.z80.iff1 = testIn->iff1;
        M.z80.iff2 = testIn->iff2;
        M.z80.im = testIn->im;
        M.z80.halt = testIn->halted;

        // Set up the memory
        for (i64 blk = 0; blk < arrayCount(testIn->memBlocks); ++blk)
        {
            TestBlock* b = &testIn->memBlocks[blk];
            u16 addr = b->address;
            memoryLoad(&M.memory, addr, b->bytes, (u16)arrayCount(b->bytes));
        }

#ifdef NX_DEBUG_TEST
        if (stringCompare(testIn->name, NX_DEBUG_TEST) == 0)
        {
            K_BREAK();
        }
#endif

        while (tStates < testIn->tStates)
        {
            machineUpdate(&M, &tStates);
        }

        fprintf(gResultsFile, "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
            (int)M.z80.af.r,
            (int)M.z80.bc.r,
            (int)M.z80.de.r,
            (int)M.z80.hl.r,
            (int)M.z80.af_.r,
            (int)M.z80.bc_.r,
            (int)M.z80.de_.r,
            (int)M.z80.hl_.r,
            (int)M.z80.ix.r,
            (int)M.z80.iy.r,
            (int)M.z80.sp.r,
            (int)M.z80.pc.r,
            (int)M.z80.m.r);
        fprintf(gResultsFile, "%02x %02x %d %d %d %d %d\n",
            (int)M.z80.ir.h,
            (int)M.z80.ir.l,
            (int)M.z80.iff1,
            (int)M.z80.iff2,
            (int)M.z80.im,
            (int)M.z80.halt ? 1 : 0,
            (int)tStates);

        fprintf(gResultsFile, "\n");

        machineClose(&M);
    }

    return YES;
}

void testCompare()
{
    Blob eb = blobLoad("tests.expected");
    if (!eb.bytes)
    {
        printf("\033[31;1mERROR: Cannot load 'tests.expected'\033[0m\n");
        return;
    }
    Blob rb = blobLoad("tests.results");
    if (!rb.bytes)
    {
        blobUnload(eb);
        printf("\033[31;1mERROR: Cannot load 'tests.resulted'\033[0m\n");
        return;
    }

    i64 expectedCursor = 0;
    while (expectedCursor < eb.size)
    {
        // Read name
        i64 end = expectedCursor;
        while (end < eb.size && eb.bytes[end] != '\n') ++end;
        if (end == eb.size)
        {
            printf("\033[31;1mERROR: Invalid test output!\033[0m\n");
        }
        String name = stringMakeRange(&eb.bytes[expectedCursor], &eb.bytes[end]);

        // Find end of test
        while ((end < eb.size + 1) && !(eb.bytes[end] == '\n' && eb.bytes[end + 1] == '\n')) ++end;
        if (end == eb.size)
        {
            printf("\033[31;1mERROR: Invalid test output!\033[0m\n");
        }
        end += 2;
        if (end >= rb.size)
        {
            printf("\033[31;1m[FAIL] %s!\033[0m\n", name);
            stringRelease(name);
            blobUnload(eb);
            blobUnload(rb);
            return;
        }

        // Compare expected test with results.  Should be EXACTLY the same.
        for (i64 i = expectedCursor; i < end; ++i)
        {
            if (eb.bytes[i] != rb.bytes[i])
            {
                printf("\033[31;1m[FAIL] %s!\033[0m\n", name);
                stringRelease(name);
                blobUnload(eb);
                blobUnload(rb);
                return;
            }
        }

        printf("\033[32;1m[PASS] %s!\033[0m\n", name);
        expectedCursor = end;
        stringRelease(name);
    }

    blobUnload(eb);
    blobUnload(rb);
}

#endif // NX_RUN

//----------------------------------------------------------------------------------------------------------------------
// Public API
//----------------------------------------------------------------------------------------------------------------------

bool nxOpen(Nx* N, u32* img)
{
    K_ASSERT(N);

#if NX_RUN_TESTS

    Tests T;
    if (testOpen(&T))
    {
        int count = testCount(&T);
        for (int i = 0; i < count; ++i)
        {
            if (!testRun(&T, i))
            {
                testClose(&T);
                return NO;
            }
        }
        testClose(&T);
        testCompare();
    }

    return NO;

#endif // NX_RUN_TESTS

    N->tState = 0;
    return machineOpen(&N->machine, img);
}

void nxClose(Nx* N)
{
    K_ASSERT(N);
    machineClose(&N->machine);
}

NxOut nxUpdate(Nx* N)
{
    NxOut out;

    out.redraw = NO;

    for(;;)
    {
        out.elapsedTStates = machineUpdate(&N->machine, &N->tState);
        if (N->machine.redraw)
        {
            // Interrupt occurred
            out.redraw = YES;
            break;
        }
    }

    return out;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
