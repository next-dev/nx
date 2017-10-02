//----------------------------------------------------------------------------------------------------------------------
// Video emulation of ULA
//----------------------------------------------------------------------------------------------------------------------

#pragma once

class Memory;
class Io;

// The width and height of the actual pixel area of the screen
const int kScreenWidth = 256;
const int kScreenHeight = 192;

// The width and height of the TV
// Image comprises of 64 lines of border, 192 lines of pixel data, and 56 lines of border.
// Each line comprises of 48 pixels of border, 256 pixels of pixel data, followed by another 48 pixels of border.
// Timing of a line is 24T for each border, 128T for the pixel data and 48T for the horizontal retrace (224 t-states).
const int kTvWidth = 352;
const int kTvHeight = 312;

// The width and height of the window that displays the emulated image (can be smalled than the TV size)
const int kWindowWidth = 320;
const int kWindowHeight = 256;

const int kBorderWidth = (kWindowWidth - kScreenWidth) / 2;
const int kBorderHeight = (kWindowHeight - kScreenHeight) / 2;

class Video
{
public:
    Video(Memory& memory, Io& io, u32* img);

    void render(bool flash);

private:
    Memory&     m_memory;   // Memory interface for reading VRAM
    Io&         m_io;       // I/O interface for border colour
    u32*        m_image;    // 24-bit image for storing the screen.  Should be kWindowWidth/Height in size.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

#include "io.h"

Video::Video(Memory& memory, Io& io, u32* img)
    : m_memory(memory)
    , m_io(io)
    , m_image(img)
{
}

void Video::render(bool flash)
{
    static const u32 colours[16] =
    {
        0xff000000, 0xffd70000, 0xff0000d7, 0xffd700d7, 0xff00d700, 0xffd7d700, 0xff00d7d7, 0xffd7d7d7,
        0xff000000, 0xffff0000, 0xff0000ff, 0xffff00ff, 0xff00ff00, 0xffffff00, 0xff00ffff, 0xffffffff,
    };

    if (!m_image) return;

    u32* img = m_image;
    int border = 7;

    for (int r = -kBorderHeight; r < (kScreenHeight + kBorderHeight); ++r)
    {
        if (r < 0 || r >= kScreenHeight)
        {
            for (int c = -kBorderWidth; c < (kScreenWidth + kBorderWidth); ++c)
            {
                *img++ = colours[m_io.getBorder() & 0x7];
            }
        }
        else
        {
            // Video data.
            //  Pixels address is 010S SRRR CCCX XXXX
            //  Attrs address is 0101 10YY YYYX XXXX
            //  S = Section (0-2)
            //  C = Cell row within section (0-7)
            //  R = Pixel row within cell (0-7)
            //  X = X coord (0-31)
            //  Y = Y coord (0-23)
            //
            //  ROW = SSCC CRRR
            //      = YYYY Y000
            u8 bank = 5;
            u16 p = 0x4000 + ((r & 0x0c0) << 5) + ((r & 0x7) << 8) + ((r & 0x38) << 2);
            u16 a = 0x5800 + ((r & 0xf8) << 2);

            for (int c = -kBorderWidth; c < (kScreenWidth + kBorderWidth); ++c)
            {
                if (c < 0 || c >= kScreenWidth)
                {
                    *img++ = colours[m_io.getBorder() & 0x7];
                }
                else
                {
                    i64 ts = 0;
                    // #todo: add API for direct page/address access
                    u8 data = m_memory.peek(p++, ts);
                    u8 attr = m_memory.peek(a++, ts);
                    u32 ink = 0xff000000 + colours[(attr & 7) + ((attr & 0x40) >> 3)];
                    u32 paper = 0xff000000 + colours[(attr & 0x7f) >> 3];

                    if (flash && (attr & 0x80))
                    {
                        u32 t = ink;
                        ink = paper;
                        paper = t;
                    }

                    for (int i = 7; i >= 0; --i)
                    {
                        img[i] = (data & 1) ? ink : paper;
                        data >>= 1;
                    }
                    img += 8;
                    c += 7;  // for loop with increment it once more
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
