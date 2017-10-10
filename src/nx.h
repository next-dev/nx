//----------------------------------------------------------------------------------------------------------------------
// The emulator object
// Manages a Spectrum-derived object and the UI (including the debugger)
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "spectrum.h"

#include <SFML/Graphics.hpp>

//----------------------------------------------------------------------------------------------------------------------
// Emulator class
//----------------------------------------------------------------------------------------------------------------------

class Nx
{
public:
    Nx();
    ~Nx();

    // Render the currently generated display
    void render();

    // The emulator main loop.  Will exit when the window is closed.
    void run();

    // Generate a single frame, including processing audio.
    void frame();

private:
    Spectrum*           m_machine;

    sf::RenderWindow    m_window;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
