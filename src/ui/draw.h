//----------------------------------------------------------------------------------------------------------------------
//! Defines the draw utility class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <ui/uilayer.h>

class UILayer;

//----------------------------------------------------------------------------------------------------------------------
//! The draw context
//!
//! The draw object contains information about the current state of drawing.  It will hold information such as which
//! layer is being drawn to, the area that drawing is allowed, the width and height of the draw area.  It also
//! contains the Spectrum font and can render it in cells or proportionate.
//!
//! The draw class knows how to draw to a UILayer.  The UILayer has a mock Speccy ULA display (it's described in terms
//! of pixels and attributes).

class Draw
{
public:
    Draw(UILayer& layer, int xCell, int yCell, int widthCell, int heightCell);
    Draw(const Draw& other);
    Draw& operator= (const Draw& other);

    //
    // Attributes
    //
    int getWidth() const { return m_bounds.back().m_width; }
    int getHeight() const { return m_bounds.back().m_height; }
    int getX() const { return m_bounds.back().m_x; }
    int getY() const { return m_bounds.back().m_y; }
    int getScreenWidth() const { return m_bounds.front().m_width; }
    int getScreenHeight() const { return m_bounds.front().m_height; }

    //
    // Resizing
    //
    void pushBounds(int x, int y, int width, int height);
    void popBounds();
    void pushLocalBounds(int x, int y, int width, int height);
    void setBounds(int x, int y, int width, int height);
    void pushShrink(int left, int top, int right, int bottom);
    void pushShrink(int margin) { pushShrink(margin, margin, margin, margin); }

    //
    // Level 0 - poking/low-level calculations
    // These will use local coordinates but will not check for bounds.  These are meant to be fast.
    //
    void pokePixels(int xCell, int yPixel, u8 bits);
    void andPixels(int xCell, int yPixel, u8 bits);
    void orPixels(int xCell, int yPixel, u8 bits);
    void xorPixels(int xCell, int yPixel, u8 bits);
    void pokeAttr(int xCell, int yCell, u8 attr);

    static u8 attr(Colour ink, Colour paper);

    //
    // Level 1 - Character rendering and attribute painting.
    //

    //! Print a character within a cell but without attributes.
    //!
    //! This routine sets the pixels for a character within a cell without affecting the attributes.  The character
    //! rendered is an index (c - 32) into the font array pointed to by font.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    void printChar(int xCell, int yCell, char c, const u8* font = gFont);

    //! Print a bold character within a cell but without attributes.
    //!
    //! This routine sets the pixels for a character within a cell without affecting the attributes.  The character
    //! rendered is an index (c - 32) into the font array pointed to by font.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    void printBoldChar(int xCell, int yCell, char c, const u8* font = gFont);

    //! Print a character within a cell.
    //!
    //! This routine sets the pixels for a character within a cell and paints the attribute too.  The character
    //! rendered is an index (c - 32) into the font array pointed to by font.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    void printCharAttr(int xCell, int yCell, char c, u8 attr, const u8* font = gFont);

    //! Print a bold character within a cell.
    //!
    //! This routine sets the pixels for a character within a cell and paints the attribute too.  The character
    //! rendered is an index (c - 32) into the font array pointed to by font.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    void printBoldCharAttr(int xCell, int yCell, char c, u8 attr, const u8* font = gFont);

    //! Print a character on a cell row but with X-coordinate pixel accuracy.
    //!
    //! This is used to render proportionate fonts as this routine will return the width (in pixels) of the character
    //! just rendered.  The width is automatically calculated based on the font data.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    int printPropChar(int xPixel, int yCell, char c, bool bold, const u8* font = gFont);

    //! Print a string without affecting attributes.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    //!
    //! @return     Returns the x cell coordinate after the string.
    int printString(int xCell, int yCell, const string& str, const u8* font = gFont);

    //! Print a string and set the attributes.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    //!
    //! @return     Returns the x cell coordinate after the string.
    int printString(int xCell, int yCell, const string& str, u8 attr, const u8* font = gFont);

    //! Print a bold string without affecting attributes.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    //!
    //! @return     Returns the x cell coordinate after the string.
    int printBoldString(int xCell, int yCell, const string& str, const u8* font = gFont);

    //! Print a bold string and set the attributes.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    //!
    //! @return     Returns the x cell coordinate after the string.
    int printBoldString(int xCell, int yCell, const string& str, u8 attr, const u8* font = gFont);

    //! Print a string using proportionate text.
    //!
    //! This will return the width of the string.  If you pass nullptr as the font, it will not render anything but
    //! still return the width of the string.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    int printPropString(int xCell, int yCell, const string& str, u8 attr, bool bold = false, const u8* font = gFont);

    //! Return the length (in cells) of a string rendered with the proportionate font.
    int propStringLength(const string& str, bool bold = false, const u8* font = gFont);

    //! Paint an area with attributes.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    void attrRect(int xCell, int yCell, int width, int height, u8 colour);

    //! Clear an area of pixels.
    //!
    //! These will check for bounds and not render anything if outside.  All coordinates are local to the current
    //! boundaries as defined by the current state.
    void clearRect(int xCell, int yCell, int width, int height);

    //! Wipe an area of pixels.  This means that the pixels are cleared and the attributes are set to transparent.
    void wipeRect(int xCell, int yCell, int width, int height);

    //
    // Level 2 - advanced objects
    //

    //! Renders a window with a title, fills in the background colour and optionally selects bright colours.
    void window(int xCell, int yCell, int width, int height, const string& title, bool selected,
                u8 backgroundAttr = attr(Colour::Black, Colour::White));

private:
    //! Returns true if group of 8-pixels are in bounds
    bool pixelInBounds(int xCell, int yPixel);

    //! Returns true if character cell is in bounds
    bool cellInBounds(int xCell, int yCell);

    //! Defines a rectangular area for drawing.
    struct Bounds 
    {
        int     m_x;            //!< X coordinate of the allowed rendering window.
        int     m_y;            //!< Y coordinate of the allowed rendering window.
        int     m_width;        //!< Width of the allowed rendering window.
        int     m_height;       //!< Height of the allowed rendering window.
    };

    //! Clip the given bounds with the current bounds and give actual screen bounds
    Bounds clip(Bounds bounds) const;

    //! Clip the given bounds with the current bounds and return local coordinate bounds.
    Bounds clipLocal(Bounds bounds) const;

    //! Calculate the address in the pixel array
    u8* pixelAddr(int xCell, int yPixel);

    //! Calculate the address in the attribute array
    u8* attrAddr(int xCell, int yCell);

    //! Calculate the mask, width and shift of a character based on its font's contents.
    void charInfo(const u8* font, char c, bool bold, u8& outMask, int& outLShift, int& outWidth);

private:
    u8*     m_pixels;       //!< Reference to the pixels array in a ULALayer.
    u8*     m_attrs;        //!< Reference to the attributes array in a ULALayer.
    int     m_stride;       //!< Width of the pixels and attributes array (it's really a 2D array).

    vector<Bounds>  m_bounds;   //!< Stack of bound settings.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
