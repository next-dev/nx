//----------------------------------------------------------------------------------------------------------------------
//! Defines the Overlay UI system
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <ui/uilayer.h>
#include <ui/window.h>

#include <functional>

struct FrameState;
class Nx;
class Spectrum;
class Draw;

using Key = sf::Keyboard::Key;

//----------------------------------------------------------------------------------------------------------------------
//! Keyboard event structure

struct KeyEvent
{
    Key     key;        //!< SFML keyboard code of key pressed or released.
    bool    down;       //!< True if key was pressed, false if released.
    bool    shift;      //!< True if shift key is down.
    bool    ctrl;       //!< True if ctrl key is down.
    bool    alt;        //!< True if alt key is down.

    KeyEvent(sf::Event& ev)
        : key(ev.key.code)
        , down(ev.type == sf::Event::KeyPressed)
        , shift(ev.key.shift)
        , ctrl(ev.key.control)
        , alt(ev.key.alt)
    {}

    KeyEvent(Key keyCode, bool shift, bool ctrl, bool alt)
        : key(keyCode)
        , down(true)
        , shift(shift)
        , ctrl(ctrl)
        , alt(alt)
    {}

    bool isShift() const        { return down && shift && !ctrl && !alt; }
    bool isCtrl() const         { return down && !shift && ctrl && !alt; }
    bool isAlt() const          { return down && !shift && !ctrl && alt; }
};

//----------------------------------------------------------------------------------------------------------------------
//! The menu window that is shown when Ctrl+Tab is pressed.

class MenuWindow : public Window
{
public:
    MenuWindow(Nx& nx);

    struct MenuWindowState
    {
        string                  title;          //!< Title of the menu.
        vector<string>          items;          //!< The strings shown in the menu.  Index to handler indexes this.
        function<void(int)>     handler;        //!< Handler to handle selection in menu
    };

    void apply(const MenuWindowState& state);

    //! Return the status of the menu
    bool isActivated() const;

protected:
    //! Overridden to render the menu strings
    void onRender(Draw& draw) override;

    //! Overridden to do nothing.
    bool onKey(const KeyEvent& kev) override;

    //! Overridden to do nothing.
    void onText(char ch) override {}

private:
    MenuWindowState         m_state;
    int                     m_itemsWidth;       //!< Cell width of longest entry.
    int                     m_selectedItem;     //!< Selected item in Ctrl+Tab menu or -1 if no menu.
    vector<int>             m_orderItems;       //!< The display order of the items.
};

//----------------------------------------------------------------------------------------------------------------------
//! An UI context
//!
//! An overlay is a UI context.  There can only be one overlay active at any moment.  An overlay is a layer so it
//! renders itself into a texture which is displayed in a frame.  An overlay also has windows that it can reposition
//! when the frame changes size.  An overlay can also output error messages that flash at the bottom of the screen.
//!
//! An overlay also has a context menu which can be accessed by pressing Ctrl+TAB.  What is displayed in this
//! context menu differs from overlay to overlay.

class Overlay: public UILayer
{
public:
    Overlay(Nx& nx);

    //! Apply frame changes to the overlay.
    //!
    //! Use this override to recalculate the layout for windows and other drawing elements.
    virtual void apply(const FrameState& frameState);

    //! Handle the key presses for the overlay.
    //!
    //! This method will handle the Ctrl+Tab menu or forward the keypresses to the onKey() overloaded method.
    //!
    //! @returns    True if the key was handled.  This is used to signal not to call text().
    bool key(const KeyEvent& kev);

    //! Handle the character input to the 

    //! Render the overlay.
    //!
    //! This renders the elements used in the overlay, including forwarding to various windows.
    void render(Draw& draw);

    //! Handle textual input.
    //!
    //! This should be called after key() is called, but not when key() returns true.
    void text(char ch);

    //! Set the Ctrl+Tab menu config.
    //!
    //! @param title            The title of the menu that pops up.
    //! @param menuStrings      These are the list of strings.
    //! @param handler          The function that will handle selection of the menu item.  The index
    //!                         parameter passed to the handler will be the index of the entry first passed to setMenu().
    void setMenu(string title, vector<string>&& menuStrings, function<void(int index)> handler);

    //! Show an error message.
    void error(string msg);

    //! Handler signature for key presses.
    using KeyHandler = function<void(KeyEvent)>;

    //! Add a key command.
    void addKey(string head, string desc, KeyEvent kev, KeyHandler handler);

protected:
    //! Handle the key input.
    //!
    //! The overridden routine should return true if the key was handled.
    virtual bool onKey(const KeyEvent& kev) = 0;

    //! Custom render hook.
    virtual void onRender(Draw& draw) = 0;

    //! Custom handler for textual input.
    virtual void onText(char ch) = 0;

    //! Get a reference to the emulator
    Nx& getEmulator();
    
    //! Get a constant reference to the emulator
    const Nx& getEmulator() const;

    //! Get a reference to the Spectrum
    Spectrum& getSpeccy();

    //! Get a constant reference to the Spectrum
    const Spectrum& getSpeccy() const;

private:
    Nx&     m_nx;       //!< Back reference to the emulator

    //
    // Ctrl+Tab menu
    //
    MenuWindow              m_menu;             //!< The menu window.

    //
    // Key help
    //
    struct KeyInfo
    {
        string      head;
        string      desc;
        KeyEvent    kev;
        KeyHandler  handler;

        KeyInfo() = default;
        KeyInfo(const KeyInfo&) = default;
        KeyInfo(string head, string desc, KeyEvent kev, KeyHandler handler)
            : head(head), desc(desc), kev(kev), handler(handler) {}
    };
    vector<KeyInfo>         m_keyInfos;         //!< Information on short-cut keys
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
