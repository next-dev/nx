//----------------------------------------------------------------------------------------------------------------------
//! Defines the UlaLayer class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <emulator/memory.h>
#include <video/layer.h>

//! Number of t-states in a frame.
static const int kFrameTStates = 69888;

//----------------------------------------------------------------------------------------------------------------------
//! The current video state to guide the renderer

struct VideoState
{
    TState      tStates;        //!< Number of tStates (up to 69888) that have passed this frame.
    Bank        videoBank;      //!< The current bank to that houses the VRAM.
    bool        flash;          //!< Current flash state.
    u8          borderColour;   //!< Border colour.

    VideoState()
        : tStates(kFrameTStates)
        , videoBank(MemGroup::RAM, 0)
        , flash(false)
        , borderColour(0)
    {}
};

//----------------------------------------------------------------------------------------------------------------------
//! The layer that renders the ULA screen

class UlaLayer : public Layer
{
public:
    UlaLayer(const Memory& memory);
    ~UlaLayer();

    //! Applies the current video state.
    void apply(const VideoState& videoState);

    //! Update the video texture to the current video state
    void update();

    //! Render the VRAM into pixels.
    virtual void render();

    //! Retrieve current video state/
    VideoState getVideoState() const { return m_state; }

private:
    void recalcVideoMaps();

private:
    const Memory&   m_memory;
    VideoState      m_state;
    vector<u16>     m_videoMap;     //!< Maps t-states to addresses.
    int             m_videoWrite;   //!< Write point in video.
    TState          m_startTState;  //!< Starting t-state for top-left of window.
    TState          m_drawTState;   //!< Current t-state that has been drawn to.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
