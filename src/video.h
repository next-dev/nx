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

    void frame();
    void render(bool flash, i64 tState);

private:
    Memory&             m_memory;       // Memory interface for reading VRAM
    Io&                 m_io;           // I/O interface for border colour
    u32*                m_image;        // 24-bit image for storing the screen.  Should be kWindowWidth/Height in size.
    i64                 m_tState;       // T-state drawn up to
    i64                 m_startTState;  // Starting tStates to start drawing
    std::vector<u16>    m_videoMap;     // Maps t-states to addresses
    int                 m_drawPoint;    // Draw point in 2d image
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
    , m_tState(0)
    , m_startTState(0)
    , m_videoMap(69888)
    , m_drawPoint(0)
{
    // Start of display area is 14336.  We wait 4 tstates before we draw 8 pixels.  The left border is 24 pixels wide.
    // Each scan line is 224 t-states long
    m_startTState = (14340 - 24) - (224 * kBorderHeight);

    // Initialise the t-state->address map
    //
    // Values will be:
    //      0       Do not draw
    //      1       Border colour
    //      0x4000+ Pixel address
    //
    int t = 0;

    // Vertical retrace period (and top border outside window)
    for (t = 0; t < m_startTState; ++t)
    {
        m_videoMap[t] = 0;
    }

    // Top border
    while (t < (m_startTState + (kBorderHeight * 224)))
    {
        // The line is: border (24t), screen (128t), border (24t) = 176t
        for (int i = 0; i < 176; ++i)
        {
            m_videoMap[t++] = 1;
        }

        // Horizontal retrace
        for (int i = 176; i < 224; ++i)
        {
            m_videoMap[t++] = 0;
        }
    }

    // Build middle of display
    int x = 0, y = 0;
    int addr = 2;

    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224))
    {
        // Left border
        for (int i = 0; i < 24; ++i) m_videoMap[t++] = 1;

        // Screen line
        for (int i = 24; i < 24 + 128; ++i)
        {
            // Every 4 t-states (8 pixels), we recalculate the address
            if (i % 4 == 0)
            {
                // Pixel address is 010S SRRR CCCX XXXX, where Y = SSCCCRRR
                // Attr address is  0101 10YY YYYX XXXX
                addr =
                    // Hi byte
                    // SS-------------+   RRR------+   
                    ((((y & 0xc0) >> 3) | (y & 0x07) | (0x40)) << 8) |
                    // Lo byte
                    // X-------------+   CCC-------------+
                    (((x >> 3) & 0x1f) | ((y & 0x38) << 2));
                x += 8;
            }
            m_videoMap[t++] = addr;
        }
        ++y;

        // Right border
        for (int i = 24 + 128; i < 24 + 128 + 24; ++i) m_videoMap[t++] = 1;

        // Horizontal retrace
        for (int i = 24 + 128 + 24; i < 224; ++i) m_videoMap[t++] = 0;
    }

    // Bottom border
    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224) + (kBorderHeight * 224))
    {
        for (int i = 0; i < 176; ++i) m_videoMap[t++] = 1;
        for (int i = 176; i < 224; ++i) m_videoMap[t++] = 0;
    }

    while (t < 69888)
    {
        m_videoMap[t++] = 0;
    }
}

void Video::frame()
{
    m_tState = m_startTState;
    m_drawPoint = 0;
}

void Video::render(bool flash, i64 tState)
{
    static const u32 colours[16] =
    {
        0xff000000, 0xffd70000, 0xff0000d7, 0xffd700d7, 0xff00d700, 0xffd7d700, 0xff00d7d7, 0xffd7d7d7,
        0xff000000, 0xffff0000, 0xff0000ff, 0xffff00ff, 0xff00ff00, 0xffffff00, 0xff00ffff, 0xffffffff,
    };

    // Nothing to draw yet
    if (tState < m_startTState) return;
    if (tState >= 69888)
    {
        tState = 69887;
    }

    // It takes 4 t-states to write 1 byte.
    int elapsedTStates = int(tState + 1 - m_tState);
    int numBytes = (elapsedTStates >> 2) + ((elapsedTStates % 4) > 0 ? 1 : 0);

    for (int i = 0; i < numBytes; ++i)
    {
        if (m_videoMap[m_tState] > 1)
        {
            // Pixel address
            u16 paddr = m_videoMap[m_tState];
            u8 pixelData = m_memory.peek(paddr);

            // Calculate attribute address
            // 010S SRRR CCCX XXXX --> 0101 10SS CCCX XXXX
            u16 aaddr = ((paddr & 0x1800) >> 3) + (paddr & 0x00ff) + 0x5800;
            u8 attr = m_memory.peek(aaddr);

            u8 lastPixelData = pixelData;
            u8 lastAttrData = attr;

            // Bright is either 0x08 or 0x00
            u8 bright = (attr & 0x40) >> 3;
            u8 ink = (attr & 0x07);
            u8 paper = (attr & 0x38) >> 3;
            u32 c0 = colours[paper + bright];
            u32 c1 = colours[ink + bright];

            if (flash && (attr & 0x80))
            {
                std::swap(c0, c1);
            }

            // We have a byte of pixel data and a byte of attr data - draw 8 pixels
            for (int p = 0; p < 8; ++p)
            {
                if ((pixelData & 0x80) != 0)
                {
                    m_image[m_drawPoint++] = c1;
                    lastAttrData = ink;
                }
                else
                {
                    m_image[m_drawPoint++] = c0;
                    lastAttrData = paper;
                }
                pixelData <<= 1;
            }
        } // pixel data
        else if (m_videoMap[m_tState] == 1)
        {
            u32 border = colours[m_io.getBorder()];
            for (int b = 0; b < 8; ++b)
            {
                m_image[m_drawPoint++] = border;
            }
        }

        m_tState += 4;
    } // for numbytes
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
