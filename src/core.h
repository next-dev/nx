//----------------------------------------------------------------------------------------------------------------------
//! Shared types and definitions
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstdint>
#include <config.h>

using namespace std;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using TState = i32;

#define KB(x) (1024 * (x))

#define NX_ASSERT(...) assert(__VA_ARGS__)

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>
#   define NX_BREAK() DebugBreak();
#   undef IN
#   undef OUT
#else
#   define NX_BREAK()
#endif

//----------------------------------------------------------------------------------------------------------------------
// Data access
//----------------------------------------------------------------------------------------------------------------------

#define TYPE_OF(arr, offset, type) ((type *)&arr[offset])
#define BYTE_OF(arr, offset) *TYPE_OF((arr), (offset), u8)
#define WORD_OF(arr, offset) *TYPE_OF((arr), (offset), u16)

//----------------------------------------------------------------------------------------------------------------------
// Register structure
// Useful for breaking a 16-bit value into 8-bit parts.
//----------------------------------------------------------------------------------------------------------------------

struct Reg
{
    union
    {
        u16 r;
        struct {
            u8 l;
            u8 h;
        };
    };

    Reg() : r(0) {}
    Reg(u16 x) : r(x) {}
};

//----------------------------------------------------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------------------------------------------------

template <typename T>
constexpr T alignUp(T t, T align)
{
    return (t + (align - 1)) & ~(align - 1);
}

#include <utils/format.h>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
