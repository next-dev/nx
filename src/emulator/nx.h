//----------------------------------------------------------------------------------------------------------------------
//! @brief  Nx class
//!
//! Represents the whole emulator.  It contains the spectrum, the debugger, the editor/assembler, the disassembler,
//! the guide, the configuration system and any other modules required by the product.
//!
//! @author Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <emulator/spectrum.h>
#include <overlays/overlay_emulator.h>
#include <overlays/overlay_debugger.h>
#include <video/frame.h>

//----------------------------------------------------------------------------------------------------------------------
//! Main emulator class

class Nx
{
public:
    Nx(int argc, char** argv);
    ~Nx();

    //! The emulator's main loop.
    //!
    //! Will exit when the window is closed.
    void run();

    //! Runs a frame worth of execution.
    //!
    //! This is called every 50th of a second by the audio system so the frames and audio are in synchronisation.
    void frame();

    //! Get the Spectrum machine.
    Spectrum& getSpeccy() { return m_speccy; }

    //! Get the Spectrum machine.
    const Spectrum& getSpeccy() const { return m_speccy; }

    //! Get the current overlay.
    shared_ptr<Overlay> getCurrentOverlay() const { return Overlay::getCurrentOverlay(); }

    //! Get the emulator overlay.
    shared_ptr<EmulatorOverlay> getEmulatorOverlay() { return m_emulatorOverlay; }

    //! Get the debugger overlay.
    shared_ptr<DebuggerOverlay> getDebuggerOverlay() { return m_debuggerOverlay; }

    //! Open a file (snapshot, text file, disassembly), and open the appropriate overlay.
    bool openFile(string fileName);

    //! Rebuild the layers for all the frames.  Need to do this after you change any layer.
    //! #todo: Get rid of this.
    void rebuildLayers();

    //! Set the current overlay and register an on-exit handler.  This will also rebuild the layers.
    void setOverlay(shared_ptr<Overlay> overlay, function<void()> onExit);

protected:
    //! Change the dimensions of a frame
    void setFrameDimensions(int width, int height);

    //! Load a .sna snapshot file.
    bool loadSnaSnapshot(string fileName);

    //! Load a .z80 snapshot file.
    bool loadZ80Snapshot(string fileName);

    //! Load a .nx snapshot file.
    bool loadNxSnapshot(string fileName, bool full);

    //! Save a .sna snapshot file
    bool saveSnaSnapshot(string fileName);

    //! Save a .nx snapshot file.
    bool saveNxSnapshot(string fileName, bool full);

private:
    Spectrum                        m_speccy;           //!< The machine that is emulated.
    FrameState                      m_mainFrameState;   //!< State of the main window.
    Frame                           m_mainFrame;        //!< Main window.
    bool                            m_quit;             //!< Set to true to shut down the emulator.
    shared_ptr<EmulatorOverlay>     m_emulatorOverlay;  //!< Overlay for emulator.
    shared_ptr<DebuggerOverlay>     m_debuggerOverlay;  //!< Debugger for emulator.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

