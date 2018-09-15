//----------------------------------------------------------------------------------------------------------------------
//! Implements the Spectrum machine
//!
//! @author     Matt Davis
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <emulator/memory.h>
#include <emulator/z80.h>
#include <video/ula_layer.h>

#include <functional>

class Audio;

//----------------------------------------------------------------------------------------------------------------------
//! The models supported by the emulator

enum class Model
{
    ZX48,       //!< The original 48K Spectrum.
    ZX128,      //!< The ZX Spectrum 128K.
    ZXPlus2,    //!< The ZX Spectrum +2.
    ZXNext,     //!< The ZX Spectrum Next.

    COUNT
};

//----------------------------------------------------------------------------------------------------------------------
//! Run modes
//!
//! The run modes determine if a single instruction is executed or a whole frames-worth.

enum class RunMode
{
    Stopped,        //!< Don't run any instructions.
    Normal,         //!< Emulate as normal, run as fast as possible for a frame.
    StepIn,         //!< Step over a single instruction, and follow CALLs.
    StepOver,       //!< Step over a single instruction, and run a subroutine CALL till it returns to the following instruction.
};

//----------------------------------------------------------------------------------------------------------------------
//! Defines a Spectrum machine
//!
//! Create an instance and apply a model.  If the model is different, the machine will reset with the new
//! configuration.

class Spectrum: public IExternals
{
public:
    Spectrum();
    ~Spectrum();

    //
    // Access to systems
    //

    //! Get the memory system
    Memory& getMemorySystem()                   { return m_memorySystem; }
    const Memory& getMemorySystem() const       { return m_memorySystem; }

    //! Get the video layer.
    shared_ptr<UlaLayer> getVideoLayer() const  { return m_videoLayer; }

    //! Get the audio system.
    Audio* getAudio() const                     { return m_audio.get(); }

    //! Get the CPU
    const Z80& getCpu() const                   { return m_z80; }

    //! Get the CPU
    Z80& getCpu()                               { return m_z80; }

    //
    // Application & updating
    //

    //! Applies a model to the machine.
    //!
    //! This can change the memory configuration and peripherals.  But it will reset the PC register to 0.
    void apply(Model model);

    //! Run a frame or a single instruction.
    //!
    //! This will run a single frame or instruction depending on the run mode.
    //!
    //! @returns    Returns true if a frame was complete.
    bool update(RunMode runMode);

    //! Render the whole of VRAM immediately.
    void renderVRAM();

    //
    // IExternals interface
    //

    virtual u8 peek(u16 address) override;
    virtual u8 peek(u16 address, TState& t) override;
    virtual u16 peek16(u16 address, TState& t) override;
    virtual void poke(u16 address, u8 x, TState& t) override;
    virtual void poke16(u16 address, u16 x, TState& t) override;
    virtual void contend(u16 address, TState delay, int num, TState& t) override;
    virtual u8 in(u16 port, TState& t) override;
    virtual void out(u16 port, u8 x, TState& t) override;

    //
    // Contention
    //

    //! Return true if the memory address lies in contended area of RAM.
    bool isContended(MemAddr addr) const;

    //! Return true if the I/O port is contended.
    bool isContended(u16 port) const;

    //! Return the contention for this t-state
    TState contention(TState tStates) const;

    //
    // State
    //

    //! Returns the current model the Spectrum emulator is running in.
    Model getModel() const { return m_model; }

    //! Set the border colour
    void setBorderColour(u8 colour);

    //! Return the current tState counter.
    TState getTState() const { return m_tState; }

    //! Set the tState counter
    void setTState(TState t);

    //
    // Peripherals
    //

    //! Set the keyboard state
    void applyKeyboard(vector<u8>&& keys);

private:
    //
    // CPU state
    //
    Model               m_model;            //!< Current machine model.
    TState              m_tState;           //!< T-state counter.
    Z80                 m_z80;              //!< The CPU.
    vector<TState>      m_contention;       //!< The various delays at different t-states points in a frame.

    //
    // Memory system
    //
    Memory              m_memorySystem;     //!< The memory configuration.
    array<Bank, 8>      m_slots;            //!< Our current slot configuration.

    //
    // Video system
    //
    shared_ptr<UlaLayer>    m_videoLayer;
    VideoState              m_videoState;
    int                     m_frameCounter;

    //
    // Audio system
    //
    // #todo: add owner/borrowed pointer system
    unique_ptr<Audio>       m_audio;

    //
    // ULA state
    //
    vector<u8>          m_keys;             //!< The state of the 8 keyboard ports.
    u8                  m_speaker;          //!< The state of the speaker.
    u8                  m_tapeEar;          //!< The state of the tape input.
    bool                m_pagingDisabled;   //!< True if 128K paging is disabled.
    bool                m_shadowScreen;     //!< True if the shadow screen is on.

    //    
    // Peripherals
    //
    bool                m_kempstonJoystick;     //!< Kempston joystick is enabled.
    u8                  m_kempstonState;        //!< Current joystick state.

};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
