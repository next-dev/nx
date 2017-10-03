//----------------------------------------------------------------------------------------------------------------------
// Emulator UI
// Similar to the ZX Spectrum display model: monochrome pixels with 8x8 attributes.  Attribute 0xff, will be interpreted
// as transparent.  Also resolution fits the entire window and each pixel is 2x2 desktop pixels.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

static const int kUiWidth = kWindowWidth * 2;
static const int kUiHeight = kWindowHeight * 2;

class Memory;
class Z80;
class Io;

class Ui
{
public:
    Ui(u32* img, Memory& memory, Z80& z80, Io& io);

    class Draw
    {
        friend class Ui;

    public:
        Draw(std::vector<u8>& pixels, std::vector<u8>& attr);

        //
        // Level 0 - poking
        //
        void pokeAttr(int x, int y, u8 attr);
        void pokePixel(int x, int y, u8 bits);

    private:
        std::vector<u8>&    m_pixels;
        std::vector<u8>&    m_attrs;
    };

    // Clear the screen
    void clear();

    // Render the screen
    void render(std::function<void(Ui::Draw&)> draw);

private:
    u32*                m_image;
    std::vector<u8>     m_pixels;
    std::vector<u8>     m_attrs;
    Memory&             m_memory;
    Z80&                m_z80;
    Io&                 m_io;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// Ui
//----------------------------------------------------------------------------------------------------------------------

Ui::Ui(u32* img, Memory& memory, Z80& z80, Io& io)
    : m_image(img)
    , m_pixels(kUiWidth / 8 * kUiHeight)
    , m_attrs(kUiWidth / 8 * kUiHeight / 8)
    , m_memory(memory)
    , m_z80(z80)
    , m_io(io)
{

}

void Ui::clear()
{
    m_pixels.assign(kUiWidth / 8 * kUiHeight, 0x00);
    m_attrs.assign(kUiWidth / 8 * kUiHeight / 8, 0x00);
}

void Ui::render(std::function<void(Ui::Draw &)> draw)
{
    static const u32 colours[16] =
    {
        0xff000000, 0xffd70000, 0xff0000d7, 0xffd700d7, 0xff00d700, 0xffd7d700, 0xff00d7d7, 0xffd7d7d7,
        0xff000000, 0xffff0000, 0xff0000ff, 0xffff00ff, 0xff00ff00, 0xffffff00, 0xff00ffff, 0xffffffff,
    };

    Draw d(m_pixels, m_attrs);
    draw(d);

    // Convert the pixels and attrs into an image
    u32* img = m_image;
    for (int row = 0; row < kUiHeight; ++row)
    {
        u8* attrRow = m_attrs.data() + ((row >> 3) * (kUiWidth >> 3));
        u8* pixelRow = m_pixels.data() + (row * (kUiWidth >> 3));

        for (int col = 0; col < kUiWidth / 8; ++col)
        {
            u8 p = *pixelRow++;
            u8 a = *attrRow++;

            if (0 == a)
            {
                for (int bit = 0; bit < 8; ++bit) *img++ = 0x00000000;
            }
            else
            {
                u8 ink = a & 0x07;
                u8 paper = (a & 0x38) >> 3;
                u8 bright = (a & 0x40) >> 3;

                for (int bit = 0; bit < 8; ++bit)
                {
                    *img++ = (p & 0x80) != 0 ? colours[ink + bright] : colours[paper + bright];
                    p <<= 1;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------------------------------------------

Ui::Draw::Draw(std::vector<u8>& pixels, std::vector<u8>& attr)
    : m_pixels(pixels)
    , m_attrs(attr)
{

}

void Ui::Draw::pokePixel(int x, int y, u8 bits)
{
    assert(x < (kUiWidth / 8));
    assert(y < kUiHeight);
    m_pixels[y * (kUiWidth / 8) + x] = bits;
}

void Ui::Draw::pokeAttr(int x, int y, u8 attr)
{
    assert(x < (kUiWidth / 8));
    assert(y < (kUiHeight / 8));
    m_attrs[y * (kUiWidth / 8) + x] = attr;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
