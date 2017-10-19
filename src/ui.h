//----------------------------------------------------------------------------------------------------------------------
// UI system
// Manages a Spectrum-like VRAM for use by the debugger.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "config.h"
#include "types.h"

#include <functional>
#include <vector>

#include <SFML/Graphics.hpp>

class Spectrum;

//----------------------------------------------------------------------------------------------------------------------
// Colours
//----------------------------------------------------------------------------------------------------------------------

enum class Colour
{
    Black,
    Blue,
    Red,
    Magenta,
    Green,
    Cyan,
    Yellow,
    White
};

//----------------------------------------------------------------------------------------------------------------------
// Draw class
//----------------------------------------------------------------------------------------------------------------------

extern const u8 gFont[768];
extern const u8 gGfxFont[];

class Draw
{
    friend class Ui;

public:
    Draw(vector<u8>& pixels, vector<u8>& attr);

    //
    // Level 0 - poking/low-level calculations
    //
    void pokePixel(int xCell, int yPixel, u8 bits);
    void andPixel(int xCell, int yPixel, u8 bits);
    void orPixel(int xCell, int yPixel, u8 bits);
    void xorPixel(int xCell, int yPixel, u8 bits);
    void pokeAttr(int xCell, int yCell, u8 attr);
    static u8 attr(Colour ink, Colour paper, bool bright);
    static string format(const char* format, ...);

    //
    // Level 1 - character rendering & attr painting
    // Font = 0 is the normal ROM font.
    //
    void printChar(int xCell, int yCell, char c, u8 attr, const u8* font = gFont);
    int printChar(int xPixel, int yCell, char c, const u8* font = gFont);        // X is in pixels, returns width of character
    int printString(int xCell, int yCell, const string& str, u8 attr, const u8* font = gFont);
    int printSquashedString(int xCell, int yCell, const string& str, u8 attr, const u8* font = gFont);
    int squashedStringWidth(const string& str, const u8* font = gFont);
    void attrRect(int xCell, int yCell, int width, int height, u8 colour);

    //
    // Level 2 - advanced objects
    //
    void window(int xCell, int yCell, int width, int height, const string& title, bool bright, u8 backgroundAttr = 0xf8);

private:
    void charInfo(const u8* font, char c, u8& outMask, int& lShift, int& width);

private:
    vector<u8>&     m_pixels;
    vector<u8>&     m_attrs;
};

//----------------------------------------------------------------------------------------------------------------------
// Overlay
// An overlay is a editing context.  There can only be one overlay active at any moment.  It controls the keys and
// renders into the UI.
//----------------------------------------------------------------------------------------------------------------------

class Nx;

class Overlay
{
public:
    Overlay(Nx& nx);

    void toggle(Overlay& fallbackOverlay);
    void select();
    void selectIf(bool selectCond, Overlay& fallbackOverlay);

    virtual void render(Draw& draw) = 0;
    virtual void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) = 0;
    virtual const vector<string>& commands() const { static vector<string> vs; return vs; }

    static Overlay* currentOverlay() { return ms_currentOverlay; }

protected:
    Nx& getEmulator();
    Spectrum& getSpeccy();

private:
    static Overlay*     ms_currentOverlay;
    Nx&                 m_nx;
};

//----------------------------------------------------------------------------------------------------------------------
// Ui class
//----------------------------------------------------------------------------------------------------------------------

class Ui
{
public:
    Ui(Spectrum& speccy);

    // Overlay selection
    void toggle(Overlay& overlay);
    void select(Overlay& overlay);

    // Clear the screen
    void clear();

    // Render the screen
    void render(bool flash);

    // UI sprite
    sf::Sprite& getSprite();

    // VRAM
    vector<u8>& getPixels() { return m_pixels; }
    vector<u8>& getAttrs() { return m_attrs; }

private:
    // Video state
    u32*            m_image;
    sf::Texture     m_uiTexture;
    sf::Sprite      m_uiSprite;
    vector<u8>      m_pixels;
    vector<u8>      m_attrs;
    Spectrum&       m_speccy;
};

//----------------------------------------------------------------------------------------------------------------------
// Window class
//----------------------------------------------------------------------------------------------------------------------

class Window
{
public:
    Window(Spectrum& speccy, int x, int y, int width, int height, std::string title, Colour ink, Colour paper, bool bright);

    virtual void draw(Draw& draw);
    virtual void keyPress(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt);

protected:
    //
    // Hooks
    //
    virtual void onDraw(Draw& draw) = 0;
    virtual void onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) = 0;

    //
    // Helper functions
    //

protected:
    Spectrum&       m_speccy;
    int             m_x;
    int             m_y;
    int             m_width;
    int             m_height;
    std::string     m_title;
    u8              m_bkgColour;
};

//----------------------------------------------------------------------------------------------------------------------
// Selectable window
//----------------------------------------------------------------------------------------------------------------------

class SelectableWindow : public Window
{
public:
    SelectableWindow(Spectrum& speccy, int x, int y, int width, int height, std::string title, Colour ink, Colour paper);

    void Select();
    bool isSelected() const { return ms_currentWindow == this; }

    void draw(Draw& draw) override;
    void keyPress(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) override;

    static SelectableWindow& getSelected() { return *ms_currentWindow; }

private:
    static SelectableWindow* ms_currentWindow;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
