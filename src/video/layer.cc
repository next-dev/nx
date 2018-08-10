//----------------------------------------------------------------------------------------------------------------------
//! Implements the Layer class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <video/layer.h>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// L A Y E R
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

Layer::Layer()
    : m_state()
    , m_texture()
    , m_sprite()
    , m_image(nullptr)
{

}

Layer::~Layer()
{
    delete[] m_image;
}

void Layer::apply(const LayerState& state)
{
    if (state.width != m_state.width ||
        state.height != m_state.height)
    {
        m_texture.create(state.width, state.height);
        m_sprite.setTexture(m_texture);
        resizeImage(state.width, state.height);
    }

    m_state = state;
}

sf::Sprite& Layer::getSprite()
{
    m_texture.update((const u8*)getImage());
    return m_sprite;
}

void Layer::resizeImage(int width, int height)
{
    delete[] m_image;
    m_image = new u32[width * height];
    for (int i = 0; i < width * height; ++i)
    {
        m_image[i] = 0;
    }
}

