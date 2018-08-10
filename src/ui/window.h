//----------------------------------------------------------------------------------------------------------------------
//! Defines the Window class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <ui/uilayer.h>

//----------------------------------------------------------------------------------------------------------------------
//! Represents a window view in the UI.
//!
//! A window can handle it's own key presses and drawing, but requires the overlay owning the window to pass control
//! to it via key(), render(), and text().  Windows can override the onKey(), onRender() & onText() to customise
//! what happens.
//!
//! The Draw class passed to the Window has its drawing area reduced by the Window class by the time it gets to
//! the onRender method.

class Nx;
struct KeyEvent;

class Window
{
public:
    Window(Nx& nx);

    struct State
    {
        string      title;      //!< Title shown on the window.
        int         x;          //!< X coordinate of top-left corner of window.
        int         y;          //!< Y coordinate of top-left corner of window.
        int         width;      //!< Width of window in cells.
        int         height;     //!< Height of window in cells.
        Colour      ink;        //!< Ink colour of window background.
        Colour      paper;      //!< Paper colour of window background.
        bool        selected;   //!< Is the window selected?
    };

    //! Apply the state to the window.
    void apply(const State& state);

    //! Draw the window.
    //!
    //! This method draws the window frame, according to the state.  It then adjusts the Draw to set the
    //! draw area within the window itself and passes it to onRender();
    void render(Draw& draw);

    //! Called by the overlay to handle key presses.
    //!
    //! If the key is handled, this returns true and so the text handler does not need to be called.
    bool key(const KeyEvent& kev);

    //! Called by the overlay to handle textual input.
    void text(char ch);

    //! Return the state.
    State& getState() { return m_currentState; }

    //! Return the state.
    const State& getState() const { return m_currentState; }

protected:
    //! Override to fill in the contents of the window.
    virtual void onRender(Draw& draw) = 0;

    //! Override to handle key presses and return true if handled.
    virtual bool onKey(const KeyEvent& kev) = 0;

    //! Override to handle textual input.
    virtual void onText(char ch) = 0;

    //! Get a reference to the emulator.
    Nx& getEmulator() { return m_nx; }

    //! Get a reference to the emulator.
    const Nx& getEmulator() const { return m_nx; }

private:
    Nx&     m_nx;
    State   m_currentState;
};
