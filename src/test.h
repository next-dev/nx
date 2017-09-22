//----------------------------------------------------------------------------------------------------------------------
// Test system
// Manages simple machines with just Z80 and Memory.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "memory.h"
#include "z80.h"
#include <kore/k_memory.h>
#include <kore/k_string.h>

typedef struct  
{
    u16         address;
    Array(u8)   bytes;
}
TestBlock;

typedef struct
{
    u16                 af, bc, de, hl;
    u16                 af_, bc_, de_, hl_;
    u16                 ix, iy, sp, pc, mp;
    u8                  i, r;
    u8                  iff1, iff2, im;
    u8                  halted;
    i64                 tStates;
}
MachineState;

typedef struct  
{
    String              name;
    MachineState        state;
    Array(TestBlock)    memBlocks;
}
TestIn;

typedef enum
{
    TO_MemoryRead,
    TO_MemoryWrite,
    TO_MemoryContend,
    TO_PortRead,
    TO_PortWrite,
    TO_PortContend
}
TestOp;

typedef struct
{
    i64         time;
    TestOp      op;
    u16         address;
    u8          data;
}
TestEvent;

typedef struct  
{
    String              name;
    MachineState        state;
    Array(TestEvent)    events;
    Array(TestBlock)    memBlocks;
}
TestResult;

typedef struct  
{
    Array(TestIn) tests;
    Array(TestResult) results;
}
Tests;

typedef struct 
{
    Memory  memory;
    Z80     cpu;
}
TestMachine;

// Load the unit tests and open a console for output
bool testOpen(Tests* T);

// Release memory associated with the unit tests.
void testClose(Tests* T);

// Get the number of tests
int testCount(Tests* T);

// Run a test
bool testRun(Tests* T, int index);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#define NX_IS_WHITESPACE(c) ((c) <= 32)
#define NX_FOR_INPUT(s, e) while(*(s) < (e) && !NX_IS_WHITESPACE(**(s)))

internal void testSkipWhitespace(const u8** s, const u8* e)
{
    while (*s < e && NX_IS_WHITESPACE(**s))
    {
        (*s)++;
    }
}

internal void testNextLine(const u8** s, const u8* e)
{
    while (*s < e && NX_IS_WHITESPACE(**s) && (**s != '\n'))
    {
        (*s)++;
    }

    // Skip the \n
    (*s)++;
}

internal bool testParseName(const u8** s, const u8* e, String* outStr)
{
    testSkipWhitespace(s, e);
    const u8* start = *s;
    NX_FOR_INPUT(s, e)
    {
        (*s)++;
    }

    *outStr = stringMakeRange((const i8*)start, (const i8*)*s);
    return K_BOOL(stringSize(*outStr) != 0);
}

internal bool testUpdateHex(u8 ch, u16* out)
{
    *out <<= 4;
    if (ch >= '0' && ch <= '9') *out += (ch - '0');
    else if (ch >= 'a' && ch <= 'f') *out += (ch - 'a' + 10);
    else return NO;

    return YES;
}

internal bool testParseHex16(const u8** s, const u8* e, u16* out)
{
    testSkipWhitespace(s, e);
    *out = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (*s == e) return NO;
        if (!testUpdateHex(*((*s)++), out)) return NO;
    }
    if (!NX_IS_WHITESPACE(**s)) return NO;

    return YES;
}

internal bool testParseHex8(const u8** s, const u8* e, u8* out)
{
    testSkipWhitespace(s, e);
    u16 x = 0;
    for (int i = 0; i < 2; ++i)
    {
        if (*s == e) return NO;
        if (!testUpdateHex(*((*s)++), &x)) return NO;
    }
    if (!NX_IS_WHITESPACE(**s)) return NO;
    *out = (u8)x;

    return YES;
}

internal bool testParseInt1(const u8** s, const u8* e, u8* out)
{
    testSkipWhitespace(s, e);
    if (**s >= '0' && **s <= '9')
    {
        *out = **s - '0';
    }
    else
    {
        return NO;
    }

    *s += 1;
    return YES;
}

internal bool testParseInt(const u8** s, const u8* e, i64* out)
{
    testSkipWhitespace(s, e);
    *out = 0;
    i64 minus = 1;
    if (**s == '-')
    {
        minus = -1;
        (*s)++;
    }
    NX_FOR_INPUT(s, e)
    {
        if (**s >= '0' && **s <= '9')
        {
            *out *= 10;
            *out += (**s - '0');
        }
        else if (NX_IS_WHITESPACE(**s))
        {
            break;
        }
        else
        {
            return NO;
        }

        (*s)++;
    }

    *out *= minus;
    return YES;
}

internal bool testParseError(TestIn* t)
{
    printf("\033[31;1mERROR: (%s) Could not parse test!\033[0m\n", t->name);
    return NO;
}

internal bool testParseResultError(TestResult* r)
{
    printf("\033[31;1mERROR: (%s) Could not parse test result!\033[0m\n", r->name);
    return NO;
}

internal bool testCheckEnd(const u8** s, const u8* e)
{
    if ((*s + 2) > e) return NO;
    if (**s == '-' && *(*s + 1) == '1')
    {
        *s += 2;
        return YES;
    }

    return NO;
}

internal bool testParseMemBlock(const u8** s, const u8* e, TestIn* t)
{
    testSkipWhitespace(s, e);
    if (testCheckEnd(s, e)) return NO;

    TestBlock* blk = arrayExpand(t->memBlocks, 1);
    blk->bytes = 0;

    // Read address
    if (!testParseHex16(s, e, &blk->address)) return testParseError(t);

    // Keep reading bytes until we reach -1
    for(;;)
    {
        testSkipWhitespace(s, e);
        if (testCheckEnd(s, e)) break;
        u8* byte = arrayExpand(blk->bytes, 1);
        if (!testParseHex8(s, e, byte)) return testParseError(t);
    }

    return YES;
}

internal bool testParseMemBlockResult(const u8** s, const u8* e, TestResult* r)
{
    testNextLine(s, e);
    if (**s == '\n') return NO;

    TestBlock* blk = arrayExpand(r->memBlocks, 1);
    blk->bytes = 0;

    // Read address
    if (!testParseHex16(s, e, &blk->address)) return testParseResultError(r);

    // Keep reading bytes until we reach -1
    for (;;)
    {
        testSkipWhitespace(s, e);
        if (testCheckEnd(s, e)) break;
        u8* byte = arrayExpand(blk->bytes, 1);
        if (!testParseHex8(s, e, byte)) return testParseResultError(r);
    }

    return YES;
}

internal bool testParseOp(const u8** s, const u8* e, TestResult* r)
{
    testNextLine(s, e);
    if (**s != ' ') return NO;

    // Read the time stamp
    TestEvent* ev = arrayExpand(r->events, 1);
    if (!testParseInt(s, e, &ev->time)) return NO;

    // Read the operation
    testSkipWhitespace(s, e);
    int op = 0;
    if (*s + 2 > e) return NO;
    if (**s == 'M') op = 0;
    else if (**s == 'P') op = 3;
    else return NO;
    (*s)++;
    if (**s == 'R') op += 0;
    else if (**s == 'W') op += 1;
    else if (**s == 'C') op += 2;
    else return NO;
    (*s)++;
    ev->op = (TestOp)op;

    // Read the address
    if (!testParseHex16(s, e, &ev->address)) return NO;

    // Read the data (if required)
    if (op % 3 != 2)
    {
        if (!testParseHex8(s, e, &ev->data)) return NO;
    }
    else
    {
        ev->data = 0;
    }

    return YES;
}

bool testOpen(Tests* T)
{
    TestIn* t = 0;
    TestResult* r = 0;
    T->tests = 0;
    T->results = 0;
    windowConsole();

    int phase = 0;

    // Read in the tests
    Blob b = blobLoad("tests.in");
    if (b.bytes)
    {
        const u8* s = b.bytes;
        const u8* e = s + b.size;
        while (s < e)
        {
            // Format of tests are:
            //
            //  1:  Name
            //  2:  AF BC DE HL AF' BC' DE' HL' IX IY SP PC MP
            //  3:  I R IFF1 IFF2 IM halted     tStates
            //  4+: ADDR BB BB ... BB -1
            //      ...
            //  N:  -1
            t = arrayExpand(T->tests, 1);
            t->memBlocks = 0;
            if (!testParseName(&s, e, &t->name)) goto error;

            if (!testParseHex16(&s, e, &t->state.af)) goto error;
            if (!testParseHex16(&s, e, &t->state.bc)) goto error;
            if (!testParseHex16(&s, e, &t->state.de)) goto error;
            if (!testParseHex16(&s, e, &t->state.hl)) goto error;
            if (!testParseHex16(&s, e, &t->state.af_)) goto error;
            if (!testParseHex16(&s, e, &t->state.bc_)) goto error;
            if (!testParseHex16(&s, e, &t->state.de_)) goto error;
            if (!testParseHex16(&s, e, &t->state.hl_)) goto error;
            if (!testParseHex16(&s, e, &t->state.ix)) goto error;
            if (!testParseHex16(&s, e, &t->state.iy)) goto error;
            if (!testParseHex16(&s, e, &t->state.sp)) goto error;
            if (!testParseHex16(&s, e, &t->state.pc)) goto error;
            if (!testParseHex16(&s, e, &t->state.mp)) goto error;
            if (!testParseHex8(&s, e, &t->state.i)) goto error;
            if (!testParseHex8(&s, e, &t->state.r)) goto error;
            if (!testParseInt1(&s, e, &t->state.iff1)) goto error;
            if (!testParseInt1(&s, e, &t->state.iff2)) goto error;
            if (!testParseInt1(&s, e, &t->state.im)) goto error;
            if (!testParseInt1(&s, e, &t->state.halted)) goto error;
            if (!testParseInt(&s, e, &t->state.tStates)) goto error;

            while (testParseMemBlock(&s, e, t));

            testSkipWhitespace(&s, e);
        }
    }
    else
    {
        printf("\033[31;1mERROR: Cannot load 'tests.in'!\033[0m\n");
        return NO;
    }
    blobUnload(b);

    phase = 1;

    // Read in the expected results.
    b = blobLoad("tests.expected");
    if (b.bytes)
    {
        const u8* s = b.bytes;
        const u8* e = s + b.size;
        while (s < e)
        {
            // Format of results are:
            //
            //  1:      Name
            //  2+:     Event
            //          <Time> <Operation> <Address> <Data>?
            //  N:      AF BC DE HL AF' BC' DE' HL' IX IY SP PC MP
            //  N+1:    I R IFF1 IFF2 IM halted tStates
            //  N+2+:   Memory blocks?
            //          ...
            // <Blank line>
            //
            r = arrayExpand(T->results, 1);
            r->events = 0;
            r->memBlocks = 0;

            if (!testParseName(&s, e, &r->name)) goto error;
            while (testParseOp(&s, e, r));

            if (!testParseHex16(&s, e, &r->state.af)) goto error;
            if (!testParseHex16(&s, e, &r->state.bc)) goto error;
            if (!testParseHex16(&s, e, &r->state.de)) goto error;
            if (!testParseHex16(&s, e, &r->state.hl)) goto error;
            if (!testParseHex16(&s, e, &r->state.af_)) goto error;
            if (!testParseHex16(&s, e, &r->state.bc_)) goto error;
            if (!testParseHex16(&s, e, &r->state.de_)) goto error;
            if (!testParseHex16(&s, e, &r->state.hl_)) goto error;
            if (!testParseHex16(&s, e, &r->state.ix)) goto error;
            if (!testParseHex16(&s, e, &r->state.iy)) goto error;
            if (!testParseHex16(&s, e, &r->state.sp)) goto error;
            if (!testParseHex16(&s, e, &r->state.pc)) goto error;
            if (!testParseHex16(&s, e, &r->state.mp)) goto error;
            if (!testParseHex8(&s, e, &r->state.i)) goto error;
            if (!testParseHex8(&s, e, &r->state.r)) goto error;
            if (!testParseInt1(&s, e, &r->state.iff1)) goto error;
            if (!testParseInt1(&s, e, &r->state.iff2)) goto error;
            if (!testParseInt1(&s, e, &r->state.im)) goto error;
            if (!testParseInt1(&s, e, &r->state.halted)) goto error;
            if (!testParseInt(&s, e, &r->state.tStates)) goto error;

            while (testParseMemBlockResult(&s, e, r));

            testSkipWhitespace(&s, e);
        }
    }
    else
    {
        printf("\033[31;1mERROR: Cannot load 'tests.expected'!\033[0m\n");
        return NO;
    }

    return YES;

error:
    if (phase == 0) testParseError(t); else testParseResultError(r);
    blobUnload(b);
    testClose(T);
    return NO;
}

void testClose(Tests* T)
{
    for (int i = 0; i < arrayCount(T->tests); ++i)
    {
        for (int j = 0; j < arrayCount(T->tests[i].memBlocks); ++j)
        {
            arrayRelease(T->tests[i].memBlocks[j].bytes);
        }
        arrayRelease(T->tests[i].memBlocks);
        stringRelease(T->tests[i].name);
    }

    for (int i = 0; i < arrayCount(T->results); ++i)
    {
        arrayRelease(T->results[i].events);
        for (int j = 0; j < arrayCount(T->results[i].memBlocks); ++j)
        {
            arrayRelease(T->results[i].memBlocks[j].bytes);
        }
        arrayRelease(T->results[i].memBlocks);
        stringRelease(T->results[i].name);
    }

    arrayRelease(T->tests);
    arrayRelease(T->results);
}

int testCount(Tests* T)
{
    return (int)arrayCount(T->tests);
}

MACHINE_EVENT(testEnd)
{
    return NO;
}

bool testRun(Tests* T, int index)
{
    TestIn* testIn = &T->tests[index];

    // Create a machine with no display.
    Machine M;
    if (machineOpen(&M, 0))
    {
        i64 tState = 0;
        machineAddEvent(&M, testIn->state.tStates, &testEnd);

        while (tState < testIn->state.tStates)
        {
            tState = machineUpdate(&M, tState);
        }

        // #todo: Compare with results

        machineClose(&M);
    }

    return YES;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
