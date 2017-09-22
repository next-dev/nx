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
    String              name;
    u16                 af, bc, de, hl;
    u16                 af_, bc_, de_, hl_;
    u16                 ix, iy, sp, pc, mp;
    u8                  i, r;
    u8                  iff1, iff2, im;
    u8                  halted;
    i64                 tStates;
    Array(TestBlock)    testBlocks;
}
TestIn;

typedef struct  
{
    int _;
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
    while (*(s) < (e) && NX_IS_WHITESPACE(**(s)))
    {
        (*s)++;
    }
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
    printf("\033[1;1mERROR: (%s) Could not parse test!\033[0m\n", t->name);
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

    TestBlock* blk = arrayExpand(t->testBlocks, 1);
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

bool testOpen(Tests* T)
{
    TestIn* t = 0;
    T->tests = 0;
    T->results = 0;
    windowConsole();

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
            t->testBlocks = 0;
            if (!testParseName(&s, e, &t->name)) goto error;

            if (!testParseHex16(&s, e, &t->af)) goto error;
            if (!testParseHex16(&s, e, &t->bc)) goto error;
            if (!testParseHex16(&s, e, &t->de)) goto error;
            if (!testParseHex16(&s, e, &t->hl)) goto error;
            if (!testParseHex16(&s, e, &t->af_)) goto error;
            if (!testParseHex16(&s, e, &t->bc_)) goto error;
            if (!testParseHex16(&s, e, &t->de_)) goto error;
            if (!testParseHex16(&s, e, &t->hl_)) goto error;
            if (!testParseHex16(&s, e, &t->ix)) goto error;
            if (!testParseHex16(&s, e, &t->iy)) goto error;
            if (!testParseHex16(&s, e, &t->sp)) goto error;
            if (!testParseHex16(&s, e, &t->pc)) goto error;
            if (!testParseHex16(&s, e, &t->mp)) goto error;
            if (!testParseHex8(&s, e, &t->i)) goto error;
            if (!testParseHex8(&s, e, &t->r)) goto error;
            if (!testParseInt1(&s, e, &t->iff1)) goto error;
            if (!testParseInt1(&s, e, &t->iff2)) goto error;
            if (!testParseInt1(&s, e, &t->im)) goto error;
            if (!testParseInt1(&s, e, &t->halted)) goto error;
            if (!testParseInt(&s, e, &t->tStates)) goto error;

            while (testParseMemBlock(&s, e, t));

            testSkipWhitespace(&s, e);
        }
    }
    else
    {
        return NO;
    }

    blobUnload(b);
    return YES;

error:
    testParseError(t);
    blobUnload(b);
    return NO;
}

void testClose(Tests* T)
{
    for (int i = 0; i < arrayCount(T->tests); ++i)
    {
        for (int j = 0; j < arrayCount(T->tests[i].testBlocks); ++j)
        {
            arrayRelease(T->tests[i].testBlocks[j].bytes);
        }
        arrayRelease(T->tests[i].testBlocks);
        stringRelease(T->tests[i].name);
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
        machineAddEvent(&M, testIn->tStates, &testEnd);

        while (tState < testIn->tStates)
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
