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

    //
    // Signals
    //

    // Clears all signals.  All the other functions set them.
    virtual void clear() = 0;

    // Signals to the host to redraw.
    virtual void redraw() = 0;
};
