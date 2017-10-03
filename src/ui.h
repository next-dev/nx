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

    };

    // Clear the screen
    void clear();

    // Render the screen
    void render(std::function<void(Ui::Draw&)> draw);

private:
    u32*        m_image;
    Memory&     m_memory;
    Z80&        m_z80;
    Io&         m_io;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

Ui::Ui(u32* img, Memory& memory, Z80& z80, Io& io)
    : m_image(img)
    , m_memory(memory)
    , m_z80(z80)
    , m_io(io)
{

}

void Ui::clear()
{
    for (int i = 0; i < kUiWidth * kUiHeight; ++i)
    {
        m_image[i] = 0x00000000;
    }
}

void Ui::render(std::function<void(Ui::Draw &)> draw)
{
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
