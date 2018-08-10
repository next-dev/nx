//----------------------------------------------------------------------------------------------------------------------
//! Implements the UILayer class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <ui/draw.h>
#include <ui/uilayer.h>

//----------------------------------------------------------------------------------------------------------------------

UILayer::UILayer()
    : m_pixels(nullptr)
    , m_attrs(nullptr)
{

}

//----------------------------------------------------------------------------------------------------------------------

UILayer::~UILayer()
{
    delete[] m_pixels;
    delete[] m_attrs;
}

//----------------------------------------------------------------------------------------------------------------------

void UILayer::apply(const FrameState& frameState)
{
    // Calculate how many cells we can have
    int scaleX = (int)floorf((float)frameState.width / float(80 * 8));
    int scaleY = (int)floorf((float)frameState.height / float(64 * 8));
    int scale = min(scaleX, scaleY);

    m_cellWidth = (frameState.width / scale) / 8;
    m_cellHeight = (frameState.height / scale) / 8;

    // Allocate the memory
    delete[] m_pixels;
    delete[] m_attrs;

    int attrSize = m_cellWidth * m_cellHeight;
    int pixelSize = attrSize * 8;

    m_pixels = new u8[pixelSize];
    m_attrs = new u8[attrSize];

    fill(m_pixels, m_pixels + pixelSize, 0);
    fill(m_attrs, m_attrs + attrSize, Draw::attr(Colour::White, Colour::Transparent));

    LayerState layerState;
    layerState.alpha = 0.9f;
    layerState.width = m_cellWidth * 8;
    layerState.height = m_cellHeight * 8;
    Layer::apply(layerState);
}

//----------------------------------------------------------------------------------------------------------------------

void UILayer::render()
{
    Draw draw(*this, 0, 0, m_cellWidth, m_cellHeight);
    render(draw);

    //
    // Convert "VRAM" into actual pixels
    //
    bool flash = false;     // #todo: add flash?
    static const u32 colours[16] =
    {
        0x00000000, 0xffd70000, 0xff0000d7, 0xffd700d7, 0xff00d700, 0xffd7d700, 0xff00d7d7, 0xffd7d7d7,
        0xff000000, 0xffff0000, 0xff0000ff, 0xffff00ff, 0xff00ff00, 0xffffff00, 0xff00ffff, 0xffffffff,
    };

    // Convert the pixels and attrs into an image
    u32* img = m_image;
    for (int row = 0; row < getHeight(); ++row)
    {
        u8* attrRow = &m_attrs[(row >> 3) * m_cellWidth];
        u8* pixelRow = &m_pixels[row * m_cellWidth];

        for (int col = 0; col < m_cellWidth; ++col)
        {
            u8 p = *pixelRow++;
            u8 a = *attrRow++;

            if (0 == a)
            {
                for (int bit = 0; bit < 8; ++bit) *img++ = 0x00000000;
            }
            else
            {
                u8 ink = a & 0x0f;
                u8 paper = (a & 0xf0) >> 4;

                if (flash && ((a & 0x80) != 0))
                {
                    swap(ink, paper);
                }

                for (int bit = 0; bit < 8; ++bit)
                {
                    *img++ = (p & 0x80) != 0 ? colours[ink] : colours[paper];
                    p <<= 1;
                }
            }
        }
    }
}
