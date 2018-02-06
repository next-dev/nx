//----------------------------------------------------------------------------------------------------------------------
// UI implementation
//----------------------------------------------------------------------------------------------------------------------

#include "ui.h"
#include "nx.h"

#include <cassert>
#include <cstdarg>

//----------------------------------------------------------------------------------------------------------------------
// Spectrum ROM font
//----------------------------------------------------------------------------------------------------------------------

const u8 gFont[768] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10,
    0x10, 0x10, 0x00, 0x10, 0x00, 0x00, 0x24, 0x24, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x24, 0x7e, 0x24, 0x24, 0x7e, 0x24, 0x00, 0x00,
    0x08, 0x3e, 0x28, 0x3e, 0x0a, 0x3e, 0x08, 0x00, 0x62, 0x64, 0x08,
    0x10, 0x26, 0x46, 0x00, 0x00, 0x10, 0x28, 0x10, 0x2a, 0x44, 0x3a,
    0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x08, 0x08, 0x08, 0x08, 0x04, 0x00, 0x00, 0x20, 0x10, 0x10, 0x10,
    0x10, 0x20, 0x00, 0x00, 0x00, 0x14, 0x08, 0x3e, 0x08, 0x14, 0x00,
    0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
    0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00, 0x3c, 0x46, 0x4a,
    0x52, 0x62, 0x3c, 0x00, 0x00, 0x18, 0x28, 0x08, 0x08, 0x08, 0x3e,
    0x00, 0x00, 0x3c, 0x42, 0x02, 0x3c, 0x40, 0x7e, 0x00, 0x00, 0x3c,
    0x42, 0x0c, 0x02, 0x42, 0x3c, 0x00, 0x00, 0x08, 0x18, 0x28, 0x48,
    0x7e, 0x08, 0x00, 0x00, 0x7e, 0x40, 0x7c, 0x02, 0x42, 0x3c, 0x00,
    0x00, 0x3c, 0x40, 0x7c, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x7e, 0x02,
    0x04, 0x08, 0x10, 0x10, 0x00, 0x00, 0x3c, 0x42, 0x3c, 0x42, 0x42,
    0x3c, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x3e, 0x02, 0x3c, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x10, 0x10, 0x20, 0x00, 0x00, 0x04, 0x08, 0x10, 0x08, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x08, 0x04, 0x08, 0x10, 0x00, 0x00, 0x3c, 0x42, 0x04, 0x08,
    0x00, 0x08, 0x00, 0x00, 0x3c, 0x4a, 0x56, 0x5e, 0x40, 0x3c, 0x00,
    0x00, 0x3c, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x00, 0x00, 0x7c, 0x42,
    0x7c, 0x42, 0x42, 0x7c, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x40, 0x42,
    0x3c, 0x00, 0x00, 0x78, 0x44, 0x42, 0x42, 0x44, 0x78, 0x00, 0x00,
    0x7e, 0x40, 0x7c, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x7e, 0x40, 0x7c,
    0x40, 0x40, 0x40, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x4e, 0x42, 0x3c,
    0x00, 0x00, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x00, 0x00, 0x3e,
    0x08, 0x08, 0x08, 0x08, 0x3e, 0x00, 0x00, 0x02, 0x02, 0x02, 0x42,
    0x42, 0x3c, 0x00, 0x00, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0x00,
    0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x42, 0x66,
    0x5a, 0x42, 0x42, 0x42, 0x00, 0x00, 0x42, 0x62, 0x52, 0x4a, 0x46,
    0x42, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00,
    0x7c, 0x42, 0x42, 0x7c, 0x40, 0x40, 0x00, 0x00, 0x3c, 0x42, 0x42,
    0x52, 0x4a, 0x3c, 0x00, 0x00, 0x7c, 0x42, 0x42, 0x7c, 0x44, 0x42,
    0x00, 0x00, 0x3c, 0x40, 0x3c, 0x02, 0x42, 0x3c, 0x00, 0x00, 0xfe,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x3c, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00,
    0x00, 0x42, 0x42, 0x42, 0x42, 0x5a, 0x24, 0x00, 0x00, 0x42, 0x24,
    0x18, 0x18, 0x24, 0x42, 0x00, 0x00, 0x82, 0x44, 0x28, 0x10, 0x10,
    0x10, 0x00, 0x00, 0x7e, 0x04, 0x08, 0x10, 0x20, 0x7e, 0x00, 0x00,
    0x0e, 0x08, 0x08, 0x08, 0x08, 0x0e, 0x00, 0x00, 0x00, 0x40, 0x20,
    0x10, 0x08, 0x04, 0x00, 0x00, 0x70, 0x10, 0x10, 0x10, 0x10, 0x70,
    0x00, 0x00, 0x10, 0x38, 0x54, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x1c, 0x22, 0x78, 0x20,
    0x20, 0x7e, 0x00, 0x00, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3c, 0x00,
    0x00, 0x20, 0x20, 0x3c, 0x22, 0x22, 0x3c, 0x00, 0x00, 0x00, 0x1c,
    0x20, 0x20, 0x20, 0x1c, 0x00, 0x00, 0x04, 0x04, 0x3c, 0x44, 0x44,
    0x3c, 0x00, 0x00, 0x00, 0x38, 0x44, 0x78, 0x40, 0x3c, 0x00, 0x00,
    0x0c, 0x10, 0x18, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x3c, 0x44,
    0x44, 0x3c, 0x04, 0x38, 0x00, 0x40, 0x40, 0x78, 0x44, 0x44, 0x44,
    0x00, 0x00, 0x10, 0x00, 0x30, 0x10, 0x10, 0x38, 0x00, 0x00, 0x04,
    0x00, 0x04, 0x04, 0x04, 0x24, 0x18, 0x00, 0x20, 0x28, 0x30, 0x30,
    0x28, 0x24, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0c, 0x00,
    0x00, 0x00, 0x68, 0x54, 0x54, 0x54, 0x54, 0x00, 0x00, 0x00, 0x78,
    0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x38, 0x44, 0x44, 0x44,
    0x38, 0x00, 0x00, 0x00, 0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x00,
    0x00, 0x3c, 0x44, 0x44, 0x3c, 0x04, 0x06, 0x00, 0x00, 0x1c, 0x20,
    0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x38, 0x40, 0x38, 0x04, 0x78,
    0x00, 0x00, 0x10, 0x38, 0x10, 0x10, 0x10, 0x0c, 0x00, 0x00, 0x00,
    0x44, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x44, 0x44, 0x28,
    0x28, 0x10, 0x00, 0x00, 0x00, 0x44, 0x54, 0x54, 0x54, 0x28, 0x00,
    0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x44,
    0x44, 0x44, 0x3c, 0x04, 0x38, 0x00, 0x00, 0x7c, 0x08, 0x10, 0x20,
    0x7c, 0x00, 0x00, 0x0e, 0x08, 0x30, 0x08, 0x08, 0x0e, 0x00, 0x00,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x70, 0x10, 0x0c,
    0x10, 0x10, 0x70, 0x00, 0x00, 0x14, 0x28, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3c, 0x42, 0x99, 0xa1, 0xa1, 0x99, 0x42, 0x3c
};

//----------------------------------------------------------------------------------------------------------------------
// Graphics font
//----------------------------------------------------------------------------------------------------------------------

const u8 gGfxFont[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // Space
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,     // Left line            !
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,     // Right line           "
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xff,     // Bottom left corner   #
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xff,     // Bottom right corner  $
    0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,     // Slope                %
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,     // Bottom line          &
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,     // Vertical bar         '
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xff,     // Upside down T        (
    0x00, 0x3c, 0x7e, 0x7e, 0x7e, 0x7e, 0x3c, 0x00,     // Breakpoint symbol    )
    0x20, 0x30, 0x38, 0x3c, 0x3c, 0x38, 0x30, 0x20,     // Right arrow          *
};

//----------------------------------------------------------------------------------------------------------------------
// Overlays
//----------------------------------------------------------------------------------------------------------------------

Overlay* Overlay::ms_currentOverlay = 0;

Overlay::Overlay(Nx& nx)
    : m_nx(nx)
{

}

void Overlay::toggle(Overlay& fallbackOverlay)
{
    selectIf(ms_currentOverlay != this, fallbackOverlay);
}

void Overlay::select()
{
    ms_currentOverlay = this;
}

void Overlay::selectIf(bool selectCond, Overlay& fallbackOverlay)
{
    if (selectCond) select();
    else fallbackOverlay.select();
}

Nx& Overlay::getEmulator()
{
    return m_nx;
}

Spectrum& Overlay::getSpeccy()
{
    return getEmulator().getSpeccy();
}

//----------------------------------------------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------------------------------------------

Draw::Draw(std::vector<u8>& pixels, std::vector<u8>& attr)
    : m_pixels(pixels)
    , m_attrs(attr)
{

}

//----------------------------------------------------------------------------------------------------------------------
// Level 0 drawing
//----------------------------------------------------------------------------------------------------------------------

void Draw::pokePixel(int xCell, int yPixel, u8 bits)
{
    assert(xCell < (kUiWidth / 8));
    assert(yPixel < kUiHeight);
    m_pixels[yPixel * (kUiWidth / 8) + xCell] = bits;
}

void Draw::andPixel(int xCell, int yPixel, u8 bits)
{
    assert(xCell < (kUiWidth / 8));
    assert(yPixel < kUiHeight);
    m_pixels[yPixel * (kUiWidth / 8) + xCell] &= bits;
}

void Draw::orPixel(int xCell, int yPixel, u8 bits)
{
    assert(xCell < (kUiWidth / 8));
    assert(yPixel < kUiHeight);
    m_pixels[yPixel * (kUiWidth / 8) + xCell] |= bits;
}

void Draw::xorPixel(int xCell, int yPixel, u8 bits)
{
    assert(xCell < (kUiWidth / 8));
    assert(yPixel < kUiHeight);
    m_pixels[yPixel * (kUiWidth / 8) + xCell] ^= bits;
}

void Draw::pokeAttr(int xCell, int yCell, u8 attr)
{
    assert(xCell < (kUiWidth / 8));
    assert(yCell < (kUiHeight / 8));
    m_attrs[yCell * (kUiWidth / 8) + xCell] = attr;
}

u8 Draw::attr(Colour ink, Colour paper, bool bright)
{
    return (bright ? 0x40 : 0x00) + (u8(paper) << 3) + u8(ink);
}

string Draw::format(const char* format, ...)
{
    char buffer[81];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 81, format, args);
    va_end(args);

    return string(buffer);
}

//----------------------------------------------------------------------------------------------------------------------
// Level 1 drawing
//----------------------------------------------------------------------------------------------------------------------

void Draw::printChar(int xCell, int yCell, char c, u8 attr, const u8* font)
{
    if (c < 32 || c > 127) c = 32;
    pokeAttr(xCell, yCell, attr);
    const u8* pixels = &font[(c - ' ') * 8];
    for (int i = 0; i < 8; ++i)
    {
        pokePixel(xCell, yCell * 8 + i, *pixels++);
    }
}

void Draw::charInfo(const u8* font, char c, u8& outMask, int& outLShift, int &outWidth)
{
    const u8* pixels = &font[(c - ' ') * 8];
    outLShift = 0;
    outMask = 0;

    // Prepare the mask by ORing all the bytes.
    // If the mask is blank (i.e. a space), create one of width 6.
    for (int i = 0; i < 8; ++i) outMask |= pixels[i];
    if (0 == outMask) outMask = 0xfc;

    // Make sure there is only 1 pixel at most to the left of the mask
    for (u8 b = 0xc0; ((outMask << outLShift) & b) == 0; ) ++outLShift;

    outWidth = 8 - outLShift;
    u8 mask = outMask;
    while ((mask & 1) == 0)
    {
        --outWidth;
        mask >>= 1;
    }
}

int Draw::printChar(int xPixel, int yCell, char c, const u8* font)
{
    // #todo: Put the mask/lshift calculations in a table
    if (xPixel < 0 || xPixel >= kUiWidth) return 0;
    if (yCell < 0 || yCell >= kUiHeight) return 0;
    if (c < 32 || c > 127) c = 32;

    // Calculate the character information (mask, left adjust and width)
    u8 mask = 0;
    int lShift = 0;
    int width = 0;
    charInfo(font, c, mask, lShift, width);

    // Make sure there is only 1 pixel at most to the left of the mask
    int rShift = xPixel % 8;
    int cx = xPixel / 8;
    int y = yCell * 8;
    const u8* pixels = &font[(c - ' ') * 8];
    for (u8 b = 0xc0; ((mask << lShift) & b) == 0; ) ++lShift;

    for (int i = 0; i < 8; ++i)
    {
        Reg m (~(mask << (8 - rShift + lShift)));
        Reg w (*pixels++ << (8 - rShift + lShift));
        andPixel(cx, y + i, m.h);
        orPixel(cx, y + i, w.h);
        if ((rShift != 0) && ((xPixel + 8) < kUiWidth))
        {
            andPixel(cx + 1, y + i, m.l);
            orPixel(cx + 1, y + i, w.l);
        }
    }

    return width;
}

int Draw::printString(int xCell, int yCell, const string& str, u8 attr, const u8* font)
{
    for (char c : str)
    {
        printChar(xCell++, yCell, c, attr, font);
        if (xCell >= (kUiWidth / 8))
        {
            ++yCell;
            xCell = 0;
            if (yCell >= (kUiHeight / 8)) return 0;
        }
    }
    return xCell;
}

int Draw::printSquashedString(int xCell, int yCell, const string& str, u8 attr, const u8* font)
{
    int maxWidth = 0;
    int x = xCell * 8;
    for (char c : str)
    {
        int w = printChar(x, yCell, c, font);
        maxWidth += w;
        x += w;
    }

    // Render the attributes as best we can
    int len = maxWidth / 8 + (maxWidth % 8 ? 1 : 0);
    for (int i = xCell; i < std::min(kUiWidth / 8, xCell + len); ++i)
    {
        pokeAttr(i, yCell, attr);
    }

    return len;
}

int Draw::squashedStringWidth(const string& str, const u8* font)
{
    int maxWidth = 0;
    u8 mask;
    int lShift;
    int w;

    for (char c : str)
    {
        charInfo(font, c, mask, lShift, w);
        maxWidth += w;
    }

    // Render the attributes as best we can
    return maxWidth / 8 + (maxWidth % 8 ? 1 : 0);
}

void Draw::attrRect(int xCell, int yCell, int width, int height, u8 colour)
{
    for (int r = yCell; r < yCell + height; ++r)
    {
        for (int c = xCell; c < xCell + width; ++c)
        {
            pokeAttr(c, r, colour);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Level 2 drawing
//----------------------------------------------------------------------------------------------------------------------

void Draw::window(int xCell, int yCell, int width, int height, const string& title, bool bright, u8 backgroundAttr)
{
    Colour titleBkgColour = Colour::Black;
    Colour titleFgColour = Colour::White;

    if ((backgroundAttr & 0x38) == 0)
    {
        // Black background - change the colours
        titleBkgColour = Colour::Blue;
    }

    int titleMaxLen = width - 7;
    assert(titleMaxLen > 0);
    int titleLen = std::min(titleMaxLen, (int)title.size());

    // Render the title
    printChar(xCell, yCell, ' ', attr(titleFgColour, titleBkgColour, bright));
    for (int i = 0; i < titleMaxLen + 1; ++i)
    {
        printChar(xCell + i, yCell, ' ', attr(titleFgColour, titleBkgColour, bright));
    }
    titleLen = printSquashedString(xCell + 1, yCell, title, attr(titleFgColour, titleBkgColour, bright));

    // Render the top-right corner of window
    int x = xCell + titleMaxLen + 1;
    printChar(x++, yCell, '%', attr(Colour::Red, titleBkgColour, bright), gGfxFont);
    printChar(x++, yCell, '%', attr(Colour::Yellow, Colour::Red, bright), gGfxFont);
    printChar(x++, yCell, '%', attr(Colour::Green, Colour::Yellow, bright), gGfxFont);
    printChar(x++, yCell, '%', attr(Colour::Cyan, Colour::Green, bright), gGfxFont);
    printChar(x++, yCell, '%', attr(titleBkgColour, Colour::Cyan, bright), gGfxFont);
    printChar(x, yCell, ' ', attr(titleFgColour, titleBkgColour, bright));

    // Render the window
    ++yCell;
    for (int row = 1; row < (height - 1); ++row, ++yCell)
    {
        int x = xCell;
        printChar(x++, yCell, '!', backgroundAttr, gGfxFont);
        for (int col = 1; col < (width - 1); ++col, ++x)
        {
            pokeAttr(x, yCell, backgroundAttr);
        }
        printChar(x, yCell, '"', backgroundAttr, gGfxFont);
    }

    // Render bottom line
    x = xCell;
    printChar(x++, yCell, '#', backgroundAttr, gGfxFont);
    for (int col = 1; col < (width - 1); ++col, ++x)
    {
        printChar(x, yCell, '&', backgroundAttr, gGfxFont);
    }
    printChar(x, yCell, '$', backgroundAttr, gGfxFont);
}

//----------------------------------------------------------------------------------------------------------------------
// UI class
//----------------------------------------------------------------------------------------------------------------------

Ui::Ui(Spectrum& speccy)
    : m_image(new u32 [kUiWidth * kUiHeight])
    , m_uiTexture()
    , m_uiSprite()
    , m_pixels(kUiWidth / 8 * kUiHeight)
    , m_attrs(kUiWidth / 8 * kUiHeight / 8)
    , m_speccy(speccy)
{
    m_uiTexture.create(kUiWidth, kUiHeight);
    m_uiSprite.setTexture(m_uiTexture);
}

void Ui::clear()
{
    m_pixels.assign(kUiWidth / 8 * kUiHeight, 0x00);
    m_attrs.assign(kUiWidth / 8 * kUiHeight / 8, 0x00);
}

void Ui::render(bool flash)
{
    //
    // Render the overlay
    //
    Draw draw(m_pixels, m_attrs);
    clear();
    if (Overlay::currentOverlay())
    {
        Overlay::currentOverlay()->render(draw);
    }

    //
    // Render the commands
    //
    const vector<string>& commands = Overlay::currentOverlay()->commands();
    if (commands.size() > 0)
    {
        int y = 63;
        int x = 0;
        u8 bkg = draw.attr(Colour::Black, Colour::White, true);
        u8 hi = draw.attr(Colour::White, Colour::Red, true);
        for (const auto& str : commands)
        {
            vector<string> ss = split(str, '|');
            int len = (int)ss[0].length() + draw.squashedStringWidth(ss[1]);
            if (x + len >= 80)
            {
                for (; x < 80; ++x) draw.printChar(x, y, ' ', bkg);
                --y;
                x = 0;
            }
            x = draw.printString(x, y, ss[0], hi);
            x += draw.printSquashedString(x, y, ss[1], bkg);
            draw.printChar(x++, y, ' ', bkg);
        }
        for (; x < 80; ++x) draw.printChar(x, y, ' ', bkg);
    }

    //
    // Convert the Ui VRAM into actual renderable pixels
    //

    static const u32 colours[16] =
    {
        0xdf000000, 0xdfd70000, 0xdf0000d7, 0xdfd700d7, 0xdf00d700, 0xdfd7d700, 0xdf00d7d7, 0xdfd7d7d7,
        0xdf000000, 0xdfff0000, 0xdf0000ff, 0xdfff00ff, 0xdf00ff00, 0xdfffff00, 0xdf00ffff, 0xdfffffff,
    };

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

                if (flash && ((a & 0x80) != 0))
                {
                    swap(ink, paper);
                }

                for (int bit = 0; bit < 8; ++bit)
                {
                    *img++ = (p & 0x80) != 0 ? colours[ink + bright] : colours[paper + bright];
                    p <<= 1;
                }
            }
        }
    }
}

sf::Sprite& Ui::getSprite()
{
    m_uiTexture.update((const sf::Uint8 *)m_image);
    return m_uiSprite;
}

//----------------------------------------------------------------------------------------------------------------------
// Window base class
//----------------------------------------------------------------------------------------------------------------------

Window::Window(Nx& nx, int x, int y, int width, int height, std::string title, Colour ink, Colour paper, bool bright)
    : m_nx(nx)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_title(title)
    , m_bkgColour((int)ink + (8 * (int)paper) + (bright ? 0x40 : 0x00))
{

}

void Window::draw(Draw& draw)
{
    draw.window(m_x, m_y, m_width, m_height, m_title.c_str(), (m_bkgColour & 0x40) != 0, m_bkgColour);
    onDraw(draw);
}

void Window::keyPress(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    onKey(key, down, shift, ctrl, alt);
}

void Window::text(char ch)
{
    onText(ch);
}

//----------------------------------------------------------------------------------------------------------------------
// SelectableWindow class
//----------------------------------------------------------------------------------------------------------------------

SelectableWindow* SelectableWindow::ms_currentWindow = 0;

SelectableWindow::SelectableWindow(Nx& nx, int x, int y, int width, int height, std::string title, Colour ink, Colour paper)
    : Window(nx, x, y, width, height, title, ink, paper, false)
{

}

void SelectableWindow::draw(Draw& draw)
{
    u8 bkg = m_bkgColour;
    if (ms_currentWindow == this)
    {
        bkg |= 0x40;
    }
    else
    {
        bkg &= ~0x40;
    }
    draw.window(m_x, m_y, m_width, m_height, m_title.c_str(), (bkg & 0x40) != 0, bkg);
    onDraw(draw);
}

void SelectableWindow::keyPress(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    if (ms_currentWindow == this)
    {
        onKey(key, down, shift, ctrl, alt);
    }
}

void SelectableWindow::Select()
{
    if (ms_currentWindow)
    {
        ms_currentWindow->m_bkgColour &= ~0x40;
    }
    if (ms_currentWindow) ms_currentWindow->onUnselected();
    ms_currentWindow = this;
    ms_currentWindow->onSelected();
    m_bkgColour |= 0x40;
}
