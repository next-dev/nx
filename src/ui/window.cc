//----------------------------------------------------------------------------------------------------------------------
//! Implements the Window base class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <ui/draw.h>
#include <ui/overlay.h>
#include <ui/window.h>

//----------------------------------------------------------------------------------------------------------------------


Window::Window(Nx& nx)
    : m_nx(nx)
    , m_promptEditor()
    , m_isPrompting(false)
{
    m_promptEditor.setData(m_promptData);
}

//----------------------------------------------------------------------------------------------------------------------

void Window::apply(const State& state)
{
    m_currentState = state;
}

//----------------------------------------------------------------------------------------------------------------------

void Window::render(Draw& draw)
{
    draw.window(
        m_currentState.x,
        m_currentState.y,
        m_currentState.width,
        m_currentState.height,
        m_currentState.title,
        m_currentState.selected,
        Draw::attr(m_currentState.ink, m_currentState.paper));

    onRender(draw);
    draw.popBounds();

    // Draw prompt
    if (m_isPrompting)
    {
        int x = m_currentState.x;
        int y = m_currentState.y;
        int w = m_currentState.width;

        u8 attr = draw.attr(Colour::White, Colour::BrightMagenta);
        draw.attrRect(x, y + 1, w, 1, attr);
        draw.clearRect(x, y + 1, w, 1);
        int width = draw.printPropString(x + 2, y + 1, m_promptString + ": ", attr, true);

        x += 2 + width;
        w -= (2 + width);

        Editor::State state;
        state.x = x;
        state.y = y + 1;
        state.width = w;
        state.height = 1;
        state.colour = attr;
        state.cursor = draw.attr(Colour::Blue, Colour::White);
        m_promptEditor.apply(state);
        m_promptEditor.render(draw);
    }
}

//----------------------------------------------------------------------------------------------------------------------

bool Window::key(const KeyEvent& kev)
{
    if (m_isPrompting)
    {
        if (kev.isNormal() && kev.key == Key::Escape)
        {
            m_isPrompting = false;
            return true;
        }
        else if (kev.key == Key::Return)
        {
            string str = m_promptData.makeString();
            if (m_requireInput == RequireInputState::No || !str.empty())
            {
                m_promptHandler(move(str));
            }
            return true;
        }
        else
        {
            return m_promptEditor.key(kev);
        }
    }
    else
    {
        return onKey(kev);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Window::text(char ch)
{
    if (m_isPrompting)
    {
        m_promptEditor.text(ch);
    }
    else
    {
        onText(ch);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Window::prompt(string promptString, string originalText, PromptHandler handler, RequireInputState requireInput)
{
    m_promptString = move(promptString);
    m_isPrompting = true;
    m_promptData.clear();
    m_promptData.insert(0, originalText);
    m_promptEditor.setData(m_promptData);
    m_promptEditor.gotoBottom();
    m_promptHandler = handler;
    m_requireInput = requireInput;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
