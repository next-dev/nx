//----------------------------------------------------------------------------------------------------------------------
//! Defines the UILayer class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <video/layer.h>

//----------------------------------------------------------------------------------------------------------------------
//! Defines the colours
//!
//! Colours allowed are similar to the Spectrum.  However, the brightness can be set individually on both paper and
//! ink.  Therefore, there is no flash.  Also, there is a grey too.
//----------------------------------------------------------------------------------------------------------------------

enum class Colour
{
    Transparent,
    Blue,
    Red,
    Magenta,
    Green,
    Cyan,
    Yellow,
    Grey,

    Black,
    BrightBlue,
    BrightRed,
    BrightMagenta,
    BrightGreen,
    BrightCyan,
    BrightYellow,
    White,
};

//----------------------------------------------------------------------------------------------------------------------
//! The spectrum font plus extra graphics
//!
//! ASCII values 32-128 are the usual Spectrum characters.  After that is special characters used by the emulator's
//! overlays.
//!
//!     Ascii code  | Description
//!     ------------|--------------------------------------------------------------------
//!     0x80        | Left line (used on left of window)
//!     0x81        | Right line (used on right of window)
//!     0x82        | Bottom left corner (used on windows)
//!     0x83        | Bottom right corner (used on windows)
//!     0x84        | Slope (used for the rainbow colours on the window title)
//!     0x85        | Bottom line (used on windows)
//!     0x86        | Vertical bar (used as a divider within windows)
//!     0x87        | Upside-down T (used where a divider meets the bottom of a window)
//!     0x88        | Little circle (used as markers and breakpoint symbols)
//!     0x89        | Right arrow (used as a cursor)
//!     0x8a        | Square (used for the flags in the debugger)
//!     0x8b        | Filled square (used for the flags in the debugger)

extern const u8 gFont[];

enum FontChars: u8
{
    FC_LEFT_LINE = 0x80,
    FC_RIGHT_LINE,
    FC_BOTTOM_LEFT,
    FC_BOTTOM_RIGHT,
    FC_SLOPE,
    FC_BOTTOM_LINE,
    FC_VERTICAL_LINE,
    FC_UPSIDE_DOWN_T,
    FC_CIRCLE,
    FC_RIGHT_ARROW,
    FC_SQUARE,
    FC_FILLED_SQUARE,
};

//----------------------------------------------------------------------------------------------------------------------
//! The UILayer produces a speccy like output that is used with various UI systems

class UILayer : public Layer
{
    friend class Draw;

public:
    UILayer();
    virtual ~UILayer();

    //! Applies the frame state to the layer so dimensions are known.
    //!
    //! The application function will try to produce a size that is as close as possible to 80x64 cell display.  This
    //! depends on the frame resolutions provided.  The emulator has 3 scales:
    //!
    //! Scale | Window Width | Window Height | Pixel Scale | UI Width (Cells) | UI Height (Cells)
    //! ------|--------------|---------------|-------------|------------------|--------------------
    //! 1     | 640          | 512           | 1           | 80               | 64
    //! 3     | 960          | 768           | 1           | 120              | 96
    //! 3     | 1280         | 1024          | 2           | 80               | 64
    void apply(const FrameState& frameState) ;

    //! Render the fake image to a real texture
    void render() override;

    //! Return the cell width of the layer.
    int getCellWidth() { return getWidth() / 8; }

    //! Return the cell height of the layer.
    int getCellHeight() { return getHeight() / 8; }

protected:
    //! Override to render something to the UI Layer.
    virtual void render(Draw& draw) = 0;

    //! Get access to the pixels (Draw class uses this).
    u8* getPixels() const { return m_pixels; }

    //! Get access to the attributes (Draw class uses this).
    u8* getAttrs() const { return m_attrs; }

    //! Get the stride of the arrays.
    int getStride() const { return m_cellWidth; }

private:
    u8*     m_pixels;
    u8*     m_attrs;
    int     m_cellWidth;
    int     m_cellHeight;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
