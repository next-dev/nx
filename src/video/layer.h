//----------------------------------------------------------------------------------------------------------------------
//! Defines the Layer class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <video/frame.h>

//----------------------------------------------------------------------------------------------------------------------
//! Represents a layer.
//!
//! A layer is a 2D texture with an associated alpha value.  A frame will render them on top of each other.

struct LayerState
{
    int                 width;      //!< Width of the image in pixels.
    int                 height;     //!< Height of the image in pixels.
    f32                 alpha;      //!< Alpha value of layer (ignored if it's the bottom layer).

    LayerState()
        : width(128)
        , height(128)
        , alpha(1.0f)
    {}

    LayerState(int width, int height, f32 alpha = 1.0f)
        : width(width)
        , height(height)
        , alpha(alpha)
    {}
};

//----------------------------------------------------------------------------------------------------------------------
//! Represents a 2D image

class Layer
{
public:
    Layer();
    virtual ~Layer();

    //! Apply a new layer state to the layer.
    void apply(const LayerState& state);

    //! Override to render to your texture
    virtual void render() = 0;

    //! Grab the sprite for rendering.
    sf::Sprite& getSprite();

    //! Grab the 2D image.
    u32* getImage() { return m_image; }

    //! Grab the 2D image.
    const u32* getImage() const { return m_image; }

    //! Get the width.
    int getWidth() const { return m_state.width; }

    //! Get the height.
    int getHeight() const { return m_state.height; }

    //! Get the alpha.
    f32 getAlpha() const { return m_state.alpha; }

private:
    friend class Frame;

    //! Set the scale (set by the frame holding the layer)
    void setScale(f32 x, f32 y) { m_sprite.setScale(sf::Vector2f(x, y)); }

protected:
    //! Resize the memory allocation to cater for an image of given dimensions.
    void resizeImage(int width, int height);

protected:
    LayerState      m_state;
    sf::Texture     m_texture;
    sf::Sprite      m_sprite;
    u32*            m_image;        //!< The 2D texture.  This is not a vector for debug performance reasons.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
