//----------------------------------------------------------------------------------------------------------------------
// Common types
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using TState = i64;

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

