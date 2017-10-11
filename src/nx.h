//----------------------------------------------------------------------------------------------------------------------
// The emulator object
// Manages a Spectrum-derived object and the UI (including the debugger)
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "spectrum.h"

#include <SFML/Graphics.hpp>
#include <map>

//----------------------------------------------------------------------------------------------------------------------
// Emulator class
//----------------------------------------------------------------------------------------------------------------------

class Nx
{
public:
    Nx(int argc, char** argv);
    ~Nx();

    // Render the currently generated display
    void render();

    // The emulator main loop.  Will exit when the window is closed.
    void run();

    // Generate a single frame, including processing audio.
    void frame();
    
    // Load a snapshot
    bool loadSnapshot(string fileName);
    
    // Settings
    void setSetting(string key, string value);
    string getSetting(string key, string defaultSetting);
    void updateSettings();
    
private:
    // Keyboard routines
    void spectrumKey(sf::Keyboard::Key key, bool down);
    void debuggerKey(sf::Keyboard::Key key);
    void calculateKeys();
    
    // File routines
    vector<u8> loadFile(string fileName);

private:
    Spectrum*           m_machine;
    vector<bool>        m_speccyKeys;
    vector<u8>          m_keyRows;
    bool                m_debugger;
    map<string, string> m_settings;

    sf::RenderWindow    m_window;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
