//----------------------------------------------------------------------------------------------------------------------
//! Defines the Frame class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
//! Represents the configuration of a frame.
//!
//! Use Frame::apply() to apply the configuration to the current one and change it.

struct FrameState
{
    string                  title;
    int                     width;
    int                     height;

    FrameState()
        : title("Untitled")
        , width(800)
        , height(600)
    {}

    FrameState(string title, int width, int height)
        : title(title)
        , width(width)
        , height(height)
    {}
};

//----------------------------------------------------------------------------------------------------------------------
//! Represents an OS window frame.
//!
//! This uses SFML to implement itself.  Each frame has a list of "layers".  Each layer is scaled up to the size
//! of the window.  All layers other than the bottom can have a defined alpha value too.

class Layer;

class Frame
{
public:
    Frame();

    //! Apply a new state to the frame
    void apply(const FrameState& state);

    //! Render the layers
    void render();

    //! Fetch an event from the frame's event queue.
    //!
    //! @param  event   Event to be written to.
    //! @returns        True if there are more events to be polled
    bool pollEvent(sf::Event& event);

    //! Clear all the layers.
    void clearLayers();

    //! Add a layer to the top.
    void addLayer(shared_ptr<Layer> layer);

    //! Remove a layer from the list of layers.
    void removeLayer(shared_ptr<Layer> layer);

    //! Calculate the scales of the layers based on the framesize so that the
    //! layers fill the whole frame.
    void setScales();
    
private:
    FrameState                  m_frameState;       //!< Current frame state.
    sf::RenderWindow            m_window;           //!< SFML window object.
    vector<shared_ptr<Layer>>   m_layers;           //!< The layers to render.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
