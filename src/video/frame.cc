//----------------------------------------------------------------------------------------------------------------------
//! Implements the Frame class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <video/frame.h>
#include <video/layer.h>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// F R A M E
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

Frame::Frame()
    : m_window(sf::VideoMode(kWindowWidth * kDefaultScale, kWindowHeight * kDefaultScale), "",
        sf::Style::Titlebar | sf::Style::Close)
{

}

//----------------------------------------------------------------------------------------------------------------------

void Frame::apply(const FrameState& state)
{
    if (state.width != m_frameState.width ||
        state.height != m_frameState.height)
    {
        m_window.setSize(sf::Vector2u((unsigned)state.width, (unsigned)state.height));
        m_window.setView(sf::View(sf::FloatRect{0, 0, (f32)state.width, (f32)state.height}));
        m_frameState.width = state.width;
        m_frameState.height = state.height;
    }

    if (state.title != m_frameState.title)
    {
        m_window.setTitle(sf::String(state.title.c_str()));
        m_frameState.title = state.title;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Frame::render()
{
    m_window.clear();
    for (const auto& layer : m_layers)
    {
        layer->render();
        sf::Sprite& sprite = layer->getSprite();
        m_window.draw(sprite);
    }
    m_window.display();
}

//----------------------------------------------------------------------------------------------------------------------

bool Frame::pollEvent(sf::Event& event)
{
    return m_window.pollEvent(event);
}

//----------------------------------------------------------------------------------------------------------------------

void Frame::clearLayers()
{
    m_layers.clear();
}

//----------------------------------------------------------------------------------------------------------------------

void Frame::addLayer(shared_ptr<Layer> layer)
{
    auto it = find(m_layers.begin(), m_layers.end(), layer);
    if (it == m_layers.end())
    {
        m_layers.push_back(layer);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Frame::removeLayer(shared_ptr<Layer> layer)
{
    auto it = find(m_layers.begin(), m_layers.end(), layer);
    if (it != m_layers.end())
    {
        m_layers.erase(it);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Frame::setScales()
{
    for (auto& layer : m_layers)
    {
        f32 dx = (f32)layer->getWidth();
        f32 dy = (f32)layer->getHeight();
        f32 sx = m_frameState.width / dx;
        f32 sy = m_frameState.height / dy;

        layer->setScale(sx, sy);
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
