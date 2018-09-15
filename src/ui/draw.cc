//----------------------------------------------------------------------------------------------------------------------
//! Implements the Draw class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <ui/draw.h>

//----------------------------------------------------------------------------------------------------------------------

Draw::Draw(UILayer& layer, int xCell, int yCell, int widthCell, int heightCell)
    : m_pixels(layer.getPixels())
    , m_attrs(layer.getAttrs())
    , m_stride(layer.getStride())
{
    m_bounds.push_back(Bounds{ xCell, yCell, widthCell, heightCell });
}

//----------------------------------------------------------------------------------------------------------------------

Draw::Draw(const Draw& other)
    : m_pixels(other.m_pixels)
    , m_attrs(other.m_attrs)
    , m_stride(other.m_stride)
    , m_bounds(other.m_bounds)
{

}

//----------------------------------------------------------------------------------------------------------------------

Draw& Draw::operator= (const Draw& other)
{
    m_pixels = other.m_pixels;
    m_attrs = other.m_attrs;
    m_stride = other.m_stride;
    m_bounds = other.m_bounds;
    return *this;
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::pushBounds(int x, int y, int width, int height)
{
    m_bounds.push_back(clip(Bounds{ x, y, width, height }));
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::popBounds()
{
    // Check that there is no pop without a push
    NX_ASSERT(m_bounds.size() > 1);

    m_bounds.erase(m_bounds.end() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::pushLocalBounds(int x, int y, int width, int height)
{
    x += getX();
    y += getY();
    pushBounds(x, y, width, height);
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::setBounds(int x, int y, int width, int height)
{
    m_bounds.back() = Bounds{ x, y, width, height };
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::pushShrink(int left, int top, int right, int bottom)
{
    pushBounds(
        getX() + left,
        getY() + top,
        getWidth() - (left + right),
        getHeight() - (top + bottom));
}

//----------------------------------------------------------------------------------------------------------------------

bool Draw::pixelInBounds(int xCell, int yPixel)
{
    return cellInBounds(xCell, yPixel / 8);
}

//----------------------------------------------------------------------------------------------------------------------

bool Draw::cellInBounds(int xCell, int yCell)
{
    int x = xCell + getX();
    int y = yCell + getY();
    return
        (x >= getX()) &&
        (x < min(getX() + getWidth(), m_bounds.front().m_width)) &&
        (y >= getY()) &&
        (y < min(getY() + getHeight(), m_bounds.front().m_height));
}

//----------------------------------------------------------------------------------------------------------------------

u8* Draw::pixelAddr(int xCell, int yPixel)
{
    return m_pixels + ((getY() * 8) + yPixel) * m_stride + getX() + xCell;
}

//----------------------------------------------------------------------------------------------------------------------

u8* Draw::attrAddr(int xCell, int yCell)
{
    return m_attrs + (getY() + yCell) * m_stride + getX() + xCell;
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::pokePixels(int xCell, int yPixel, u8 bits)
{
    if (pixelInBounds(xCell, yPixel))
    {
        *pixelAddr(xCell, yPixel) = bits;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::andPixels(int xCell, int yPixel, u8 bits)
{
    if (pixelInBounds(xCell, yPixel))
    {
        *pixelAddr(xCell, yPixel) &= bits;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::orPixels(int xCell, int yPixel, u8 bits)
{
    if (pixelInBounds(xCell, yPixel))
    {
        *pixelAddr(xCell, yPixel) |= bits;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::xorPixels(int xCell, int yPixel, u8 bits)
{
    if (pixelInBounds(xCell, yPixel))
    {
        *pixelAddr(xCell, yPixel) ^= bits;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::pokeAttr(int xCell, int yCell, u8 attr)
{
    if (cellInBounds(xCell, yCell))
    {
        *attrAddr(xCell, yCell) = attr;
    }
}

//----------------------------------------------------------------------------------------------------------------------

u8 Draw::attr(Colour ink, Colour paper)
{
    return (u8)paper << 4 | (u8)ink;
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::printChar(int xCell, int yCell, char c, const u8* font /* = gFont */)
{
    u8 ch = (u8)c;
    if (ch < 32) ch = '.';
    font += ((ch - 32) * 8);
    int y = yCell * 8;
    for (int i = 0; i < 8; ++i, ++y)
    {
        pokePixels(xCell, y, *font++);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::printBoldChar(int xCell, int yCell, char c, const u8* font /* = gFont */)
{
    u8 ch = (u8)c;
    if (ch < 32) ch = '.';
    font += ((ch - 32) * 8);
    int y = yCell * 8;
    for (int i = 0; i < 8; ++i, ++y)
    {
        u8 p = *font++;
        p |= (p << 1);
        pokePixels(xCell, y, p);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::printCharAttr(int xCell, int yCell, char c, u8 attr, const u8* font /* = gFont */)
{
    printChar(xCell, yCell, c, font);
    pokeAttr(xCell, yCell, attr);
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::printBoldCharAttr(int xCell, int yCell, char c, u8 attr, const u8* font /* = gFont */)
{
    printBoldChar(xCell, yCell, c, font);
    pokeAttr(xCell, yCell, attr);
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::charInfo(const u8* font, char c, bool bold, u8& outMask, int& outLShift, int &outWidth)
{
    const u8* pixels = &font[(c - ' ') * 8];
    outLShift = 0;
    outMask = 0;

    // Prepare the mask by ORing all the bytes.
    // If the mask is blank (i.e. a space), create one of width 6.
    for (int i = 0; i < 8; ++i)
    {
        outMask |= pixels[i];
    }
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

    if (bold)
    {
        outMask |= (outMask << 1);
        outWidth += 1;
        //outLShift -= 1;
    }
}

//----------------------------------------------------------------------------------------------------------------------

int Draw::printPropChar(int xPixel, int yCell, char c, bool bold, const u8* font)
{
    // #todo: Put the mask/lshift calculations in a table
    // #todo: Do fast and/or pixel writes by moving bounds check outside calls.

    if (c < 32 || c > 127) c = '.';

    // Calculate the character information (mask, left adjust and width)
    u8 mask = 0;
    int lShift = 0;
    int width = 0;
    charInfo(font, c, bold, mask, lShift, width);

    // Make sure there is only 1 pixel at most to the left of the mask
    int rShift = xPixel % 8;
    int cx = xPixel / 8;
    int y = yCell * 8;
    const u8* pixels = &font[(c - ' ') * 8];
    for (u8 b = 0xc0; ((mask << lShift) & b) == 0; ) ++lShift;

    for (int i = 0; i < 8; ++i)
    {
        Reg m(~(mask << (8 - rShift + lShift)));
        u8 p = *pixels++;
        p = bold ? (p | (p << 1)) : p;
        Reg w(p << (8 - rShift + lShift));
        andPixels(cx, y + i, m.h);
        orPixels(cx, y + i, w.h);
        if (rShift != 0)
        {
            andPixels(cx + 1, y + i, m.l);
            orPixels(cx + 1, y + i, w.l);
        }
    }

    return width;
}

//----------------------------------------------------------------------------------------------------------------------

int Draw::printString(int xCell, int yCell, const string& str, const u8* font /* = gFont */)
{
    // #todo: do fast printChar to not do boundary checks.  We can do that here.
    int len = min(min(m_stride, getX() + getWidth()) - xCell, (int)str.size());
    for (int i = 0; i < len; ++i)
    {
        printChar(xCell++, yCell, str[i], font);
    }
    return xCell;
}

//----------------------------------------------------------------------------------------------------------------------

int Draw::printString(int xCell, int yCell, const string& str, u8 attr, const u8* font /* = gFont */)
{
    // #todo: do fast printChar to not do boundary checks.  We can do that here.
    int len = min(min(m_stride, getX() + getWidth()) - xCell, (int)str.size());
    for (int i = 0; i < len; ++i)
    {
        printCharAttr(xCell++, yCell, str[i], attr, font);
    }
    return xCell;
}

//----------------------------------------------------------------------------------------------------------------------

int Draw::printBoldString(int xCell, int yCell, const string& str, const u8* font /* = gFont */)
{
    // #todo: do fast printChar to not do boundary checks.  We can do that here.
    int len = min(min(m_stride, getX() + getWidth()) - xCell, (int)str.size());
    for (int i = 0; i < len; ++i)
    {
        printBoldChar(xCell++, yCell, str[i], font);
    }
    return xCell;
}

//----------------------------------------------------------------------------------------------------------------------

int Draw::printBoldString(int xCell, int yCell, const string& str, u8 attr, const u8* font /* = gFont */)
{
    // #todo: do fast printChar to not do boundary checks.  We can do that here.
    int len = min(min(m_stride, getX() + getWidth()) - xCell, (int)str.size());
    for (int i = 0; i < len; ++i)
    {
        printBoldCharAttr(xCell++, yCell, str[i], attr, font);
    }
    return xCell;
}

//----------------------------------------------------------------------------------------------------------------------

int Draw::printPropString(int xCell, int yCell, const string& str, u8 attr, bool bold, const u8* font /* = gFont */)
{
    int maxWidth = 0;
    int x = xCell * 8;
    for (char c : str)
    {
        int w = printPropChar(x, yCell, c, bold, font);
        maxWidth += w;
        x += w;
    }

    // Render the attributes as best we can
    int len = alignUp(maxWidth, 8) / 8;
    len = min(min(m_stride, getX() + getWidth()) - xCell, len);
    for (int i = 0; i < len; ++i)
    {
        pokeAttr(xCell + i, yCell, attr);
    }

    return len;
}

//----------------------------------------------------------------------------------------------------------------------

int Draw::propStringLength(const string& str, bool bold, const u8* font /* = gFont */)
{
    int maxWidth = 0;
    for (char c : str)
    {
        u8 mask;
        int shift, width;
        charInfo(font, c, bold, mask, shift, width);
        maxWidth += width;
    }

    return alignUp(maxWidth, 8) / 8;
}

//----------------------------------------------------------------------------------------------------------------------

Draw::Bounds Draw::clip(Bounds bounds) const
{
    NX_ASSERT(bounds.m_width >= 0);
    NX_ASSERT(bounds.m_height >= 0);

    // Clip to [0..width), then shift to global space
    int x0 = max(bounds.m_x, 0) + getX();                                // Clip to current bounds left edge
    int x1 = min(bounds.m_x + bounds.m_width, getWidth()) + getX();      // Clip to current bounds right edge

    // Now do it for y axis
    int y0 = max(bounds.m_y, 0) + getY();
    int y1 = min(bounds.m_y + bounds.m_height, getHeight()) + getY();

    NX_ASSERT(x1 >= x0);
    NX_ASSERT(y1 >= y0);

    return Bounds{ x0, y0, x1 - x0, y1 - y0 };
}

//----------------------------------------------------------------------------------------------------------------------

Draw::Bounds Draw::clipLocal(Bounds bounds) const
{
    NX_ASSERT(bounds.m_width >= 0);
    NX_ASSERT(bounds.m_height >= 0);

    // Clip to [0..width), then shift to global space
    int x0 = max(bounds.m_x, 0);                                // Clip to current bounds left edge
    int x1 = min(bounds.m_x + bounds.m_width, getWidth());      // Clip to current bounds right edge

    // Now do it for y axis
    int y0 = max(bounds.m_y, 0);
    int y1 = min(bounds.m_y + bounds.m_height, getHeight());

    NX_ASSERT(x1 >= x0);
    NX_ASSERT(y1 >= y0);

    return Bounds{ x0, y0, x1 - x0, y1 - y0 };
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::attrRect(int xCell, int yCell, int width, int height, u8 colour)
{
    Bounds b = clip(Bounds{ xCell, yCell, width, height });
    for (int j = b.m_y; j < b.m_y + b.m_height; ++j)
    {
        u8* attr = m_attrs + (j * m_stride) + b.m_x;
        for (int i = 0; i < b.m_width; ++i)
        {
            *attr++ = colour;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::clearRect(int xCell, int yCell, int width, int height)
{
    Bounds b = clip(Bounds{ xCell, yCell, width, height });
    height = b.m_height * 8;
    for (int j = 0; j < height; ++j)
    {
        u8* pixels = m_pixels + ((b.m_y * 8) + j) * m_stride + b.m_x;
        for (int i = 0; i < b.m_width; ++i)
        {
            *pixels++ = 0;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::wipeRect(int xCell, int yCell, int width, int height)
{
    clearRect(xCell, yCell, width, height);
    attrRect(xCell, yCell, width, height, attr(Colour::Black, Colour::Transparent));
}

//----------------------------------------------------------------------------------------------------------------------

void Draw::window(int xCell, int yCell, int width, int height, const string& title, bool selected, u8 backgroundAttr)
{
    Bounds b = clipLocal(Bounds{ xCell, yCell, width, height });
    xCell = b.m_x;
    yCell = b.m_y;
    width = b.m_width;
    height = b.m_height;
    pushLocalBounds(xCell, yCell, width, height);

    Colour titleBkgColour = Colour::Black;
    Colour titleFgColour = Colour::White;

    if (((backgroundAttr & 0xf0) >> 4) == (u8)Colour::Black)
    {
        // Black background - change the colours
        titleBkgColour = Colour::BrightBlue;
    }

    int titleMaxLen = width - 7;
    assert(titleMaxLen > 0);
    int titleLen = std::min(titleMaxLen, (int)title.size());

    // Clear the area
    clearRect(0, 0, width, height);

    // Render the title
    attrRect(0, 0, width, 1, attr(titleFgColour, titleBkgColour));
    titleLen = printPropString(1, 0, title, attr(titleFgColour, titleBkgColour), selected);

    // Render the top-right corner of window
    int x = titleMaxLen + 1;
    printCharAttr(x++, 0, FC_SLOPE, attr(Colour::BrightRed, titleBkgColour));
    printCharAttr(x++, 0, FC_SLOPE, attr(Colour::BrightYellow, Colour::BrightRed));
    printCharAttr(x++, 0, FC_SLOPE, attr(Colour::BrightGreen, Colour::BrightYellow));
    printCharAttr(x++, 0, FC_SLOPE, attr(Colour::BrightCyan, Colour::BrightGreen));
    printCharAttr(x++, 0, FC_SLOPE, attr(titleBkgColour, Colour::BrightCyan));

    // Render the window
    attrRect(0, 1, width, height-1, backgroundAttr);
    for (int row = 1; row < (height - 1); ++row)
    {
        printCharAttr(0, row, FC_LEFT_LINE, backgroundAttr);
        printCharAttr(width - 1, row, FC_RIGHT_LINE, backgroundAttr);
    }

    // Render bottom line
    printCharAttr(0, height-1, FC_BOTTOM_LEFT, backgroundAttr);
    for (int col = 1; col < (width - 1); ++col)
    {
        printCharAttr(col, height-1, FC_BOTTOM_LINE, backgroundAttr);
    }
    printCharAttr(width-1, height-1, FC_BOTTOM_RIGHT, backgroundAttr);

    setBounds(getX() + 1, getY() + 1, getWidth() - 2, getHeight() - 2);
}

//----------------------------------------------------------------------------------------------------------------------

string Draw::format(const char* f, ...)
{
    char buffer[81];

    va_list args;
    va_start(args, f);
    vsnprintf(buffer, 81, f, args);
    va_end(args);

    return string(buffer);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
