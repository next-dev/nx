//----------------------------------------------------------------------------------------------------------------------
// Video emulation of ULA
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/k_platform.h>
#include "memory.h"

// The width and height of the actual pixel area of the screen
#define NX_SCREEN_WIDTH     256
#define NX_SCREEN_HEIGHT    192
// The width and height of the TV
// Image comprises of 64 lines of border, 192 lines of pixel data, and 56 lines of border.
// Each line comprises of 48 pixels of border, 256 pixels of pixel data, followed by another 48 pixels of border.
// Timing of a line is 24T for each border, 128T for the pixel data and 48T for the horizontal retrace (224 t-states).
#define NX_TV_WIDTH         352
#define NX_TV_HEIGHT        312
// The width and height of the window that displays the emulated image (can be smalled than the TV size)
#define NX_WINDOW_WIDTH     320
#define NX_WINDOW_HEIGHT    256
#define NX_BORDER_WIDTH     ((NX_WINDOW_WIDTH - NX_SCREEN_WIDTH) / 2)
#define NX_BORDER_HEIGHT    ((NX_WINDOW_HEIGHT - NX_SCREEN_HEIGHT) / 2)

typedef struct
{
    Memory*     mem;        // Memory interface (weak pointer)
    u8*         border;     // Border colour
    u32*        img;        // 24-bit image for storing the screen.  Should be NX_WINDOW_WIDTH/HEIGHT in size.
}
Video;

// Initialise the video sub-system.
bool videoOpen(Video* video, Memory* mem, u8* border, u32* img);

// Close the video
void videoClose(Video* video);

// Render the ULA video.
void videoRenderULA(Video* video, bool flash);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

bool videoOpen(Video* video, Memory* mem, u8* border, u32* img)
{
    video->mem = mem;
    video->border = border;
    video->img = img;

    return YES;
}

void videoClose(Video* video)
{
}

void videoRenderULA(Video* video, bool flash)
{
    static const u32 colours[16] =
    {
        0x000000, 0x0000d7, 0xd70000, 0xd700d7, 0x00d700, 0x00d7d7, 0xd7d700, 0xd7d7d7,
        0x000000, 0x0000ff, 0xff0000, 0xff00ff, 0x00ff00, 0x00ffff, 0xffff00, 0xffffff,
    };

    u32* img = video->img;
    int border = 7;

    for (int r = -NX_BORDER_HEIGHT; r < (NX_SCREEN_HEIGHT + NX_BORDER_HEIGHT); ++r)
    {
        if (r < 0 || r >= NX_SCREEN_HEIGHT)
        {
            for (int c = -NX_BORDER_WIDTH; c < (NX_SCREEN_WIDTH + NX_BORDER_WIDTH); ++c)
            {
                *img++ = colours[*video->border & 0x7];
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

            for (int c = -NX_BORDER_WIDTH; c < (NX_SCREEN_WIDTH + NX_BORDER_WIDTH); ++c)
            {
                if (c < 0 || c >= NX_SCREEN_WIDTH)
                {
                    *img++ = colours[*video->border & 0x7];
                }
                else
                {
                    i64 ts = 0;
                    // #todo: add API for direct page/address access
                    u8 data = memoryPeek(video->mem, p++, &ts);
                    u8 attr = memoryPeek(video->mem, a++, &ts);
                    u32 ink = 0xff000000 + colours[(attr & 7) + ((attr & 0x40) >> 3)];
                    u32 paper = 0xff000000 + colours[(attr & 0x7f) >> 3];

                    if (flash)
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
