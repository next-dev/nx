//----------------------------------------------------------------------------------------------------------------------
// Common types
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstdint>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using TState = i64;

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
// String utilities
//----------------------------------------------------------------------------------------------------------------------

namespace {

    template<typename Out>
    void split(const std::string &s, char delim, Out result) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

