//----------------------------------------------------------------------------------------------------------------------
// Emulator configuration
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

class IHost
{
public:
    //
    // I/O
    //

    // Load data into memory.  Returns 0 if failed.
    virtual int load(std::string fileName, const u8*& outBuffer, i64& outSize) = 0;

    // Unload data from memory
    virtual void unload(int handle) = 0;
};
