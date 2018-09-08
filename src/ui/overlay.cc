//----------------------------------------------------------------------------------------------------------------------
//! Implements the Overlay base class & MenuWindow class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <emulator/nx.h>
#include <ui/overlay.h>
#include <ui/draw.h>
#include <utils/tinyfiledialogs.h>

//----------------------------------------------------------------------------------------------------------------------

MenuWindow::MenuWindow(Nx& nx)
    : Window(nx)
    , m_selectedItem(-1)
{

}

//----------------------------------------------------------------------------------------------------------------------

void MenuWindow::apply(const MenuWindowState& state)
{
    m_state = state;

    NX_ASSERT(!state.items.empty());
    m_orderItems.clear();
    for (int i = 0; i < (int)state.items.size(); ++i)
    {
        m_orderItems.push_back(i);
    }
    m_selectedItem = -1;

    m_itemsWidth = (int)state.title.size() + 7;
    for (const auto& str : state.items)
    {
        m_itemsWidth = max(m_itemsWidth, (int)str.size());
    }

    State s;
    s.title = state.title;
    s.x = 1;
    s.y = 1;
    s.width = m_itemsWidth + 4;
    s.height = (int)state.items.size() + 2;
    s.ink = Colour::Black;
    s.paper = Colour::White;
    s.selected = true;
    Window::apply(s);
}

//----------------------------------------------------------------------------------------------------------------------

void MenuWindow::onRender(Draw& draw)
{
    for (int i = 0; i < draw.getHeight(); ++i)
    {
        u8 colour = (i == m_selectedItem) ? draw.attr(Colour::White, Colour::BrightRed)
                                          : draw.attr(Colour::Black, Colour::White);
        const string& item = m_state.items[m_orderItems[i]];
        draw.printPropString(0, i, item, colour);
    }
}

//----------------------------------------------------------------------------------------------------------------------

bool MenuWindow::onKey(const KeyEvent& kev)
{
    using K = sf::Keyboard::Key;

    if (kev.isCtrl() && kev.key == K::Tab)
    {
        // Ctrl+Tab has just been pressed.
        if (m_selectedItem == -1 && !m_state.items.empty())
        {
            // Pressing it for the first time, select the 2nd item (if possible)
            m_selectedItem = 1;
        }
        else
        {
            // Press TAB again while pressing CTRL (as releasing CTRL would make this logic unreachable).
            ++m_selectedItem;
        }
        if (m_selectedItem == (int)m_state.items.size()) m_selectedItem = 0;
        return true;
    }
    else if ((m_selectedItem >= 0) && (!kev.down && !kev.shift && !kev.ctrl && !kev.alt))
    {
        // Ctrl+Tab has been released while selecting a menu item.  No action if selected the top item (as that is
        // the current option).
        if (m_selectedItem != 0)
        {
            swap(m_orderItems[0], m_orderItems[m_selectedItem]);
            m_state.handler(m_orderItems[0]);
        }
        m_selectedItem = -1;
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool MenuWindow::isActivated() const
{
    return m_selectedItem != -1;
}

//----------------------------------------------------------------------------------------------------------------------

Overlay::Overlay(Nx& nx)
    : m_nx(nx)
    //--- Ctrl+Tab menu state ---------------------------------------
    , m_menu(nx)
{

}

//----------------------------------------------------------------------------------------------------------------------

Overlay::~Overlay()
{

}

//----------------------------------------------------------------------------------------------------------------------

void Overlay::apply(const FrameState& frameState)
{
    UILayer::apply(frameState);
}

//----------------------------------------------------------------------------------------------------------------------

Nx& Overlay::getEmulator()
{
    return m_nx;
}

//----------------------------------------------------------------------------------------------------------------------

const Nx& Overlay::getEmulator() const
{
    return m_nx;
}

//----------------------------------------------------------------------------------------------------------------------

Spectrum& Overlay::getSpeccy()
{
    return getEmulator().getSpeccy();
}

//----------------------------------------------------------------------------------------------------------------------

const Spectrum& Overlay::getSpeccy() const
{
    return getEmulator().getSpeccy();
}

//----------------------------------------------------------------------------------------------------------------------

bool Overlay::key(const KeyEvent& kev)
{
    using K = sf::Keyboard::Key;

    if (m_menu.isActivated() ||
        (kev.isCtrl() && kev.key == K::Tab))
    {
        return m_menu.key(kev);
    }
    else
    {
        bool result = onKey(kev);
        if (!result && kev.key == K::Escape && kev.down)
        {
            // Handle ESCAPE if the overlay doesn't - this will exit the overlay
            exit();
            return true;
        }
        else
        {
            return result;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Overlay::exit()
{
    ms_currentOverlay.reset();
    if (ms_onExit) ms_onExit();
    ms_onExit = [] {};
}

//----------------------------------------------------------------------------------------------------------------------

void Overlay::text(char ch)
{
    if (!m_menu.isActivated())
    {
        onText(ch);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Overlay::render(Draw& draw)
{
    // First draw the custom stuff as the Ctrl+Tab menu (if needed) must be drawn over.
    onRender(draw);

    // Draw the key info
    int width = draw.getWidth();
    int y = draw.getHeight() - 1;
    int x = 0;
    u8 bkg = draw.attr(Colour::Black, Colour::White);
    u8 hi = draw.attr(Colour::White, Colour::BrightRed);
    for (const auto& info : m_keyInfos)
    {
        int len = (int)info.head.size() + draw.propStringLength(info.desc);
        if (x + len >= width)
        {
            draw.clearRect(x, y, width - x, 1);
            draw.attrRect(x, y, width - x, 1, bkg);
            --y;
            x = 0;
        }
        x = draw.printString(x, y, info.head, hi);
        x += draw.printPropString(x, y, info.desc, bkg);
        draw.printCharAttr(x++, y, ' ', bkg);
    }
    if (x != 0)
    {
        draw.clearRect(x, y, width - x, 1);
        draw.attrRect(x, y, width - x, 1, bkg);
    }

    // Now draw the menu.
    if (m_menu.isActivated()) m_menu.render(draw);
}

//----------------------------------------------------------------------------------------------------------------------

void Overlay::error(string msg)
{
    // #todo: Change this to a proper output on the overlay.
    tinyfd_messageBox("ERROR", msg.c_str(), "ok", "error", 0);
}

//----------------------------------------------------------------------------------------------------------------------

void Overlay::setMenu(string title, vector<string>&& menuStrings, function<void(int index)> handler)
{
    NX_ASSERT(!title.empty());
    NX_ASSERT(!menuStrings.empty());
    NX_ASSERT(handler);

    MenuWindow::MenuWindowState windowState;
    windowState.title = title;
    windowState.items = menuStrings;
    windowState.handler = handler;
    m_menu.apply(windowState);
}

//----------------------------------------------------------------------------------------------------------------------

void Overlay::addKey(string head, string desc, KeyEvent kev, KeyHandler handler)
{
    m_keyInfos.emplace_back(head, desc, kev, handler);
}

//----------------------------------------------------------------------------------------------------------------------

shared_ptr<Overlay> Overlay::ms_currentOverlay;
function<void()> Overlay::ms_onExit;

void Overlay::setOverlay(shared_ptr<Overlay> overlay, function<void()> onExit)
{
    ms_currentOverlay = overlay;
    ms_onExit = onExit;
}

//----------------------------------------------------------------------------------------------------------------------

bool Overlay::isCurrent() const
{
    return ms_currentOverlay.get() == this;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

