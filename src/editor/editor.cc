//----------------------------------------------------------------------------------------------------------------------
//! Implements the Editor UI
//----------------------------------------------------------------------------------------------------------------------

#include <editor/editor.h>
#include <tuple>
#include <ui/draw.h>

//----------------------------------------------------------------------------------------------------------------------
// Constructor

Editor::Editor()
    : m_state()
    , m_data(nullptr)
    , m_cursor(0)
    , m_top(0)
    , m_showLineNumbers(false)
{
}

//----------------------------------------------------------------------------------------------------------------------
// Destructor

Editor::~Editor()
{
}

//----------------------------------------------------------------------------------------------------------------------
// apply

void Editor::apply(const State& state)
{
    m_state = state;
}

//----------------------------------------------------------------------------------------------------------------------
// render

void Editor::render(Draw& draw)
{
    int width = m_state.width;
    int height = m_state.height;

    if (m_data)
    {
        auto p = m_data->getLinePos(m_top);

        for (int y = 0; y < height; ++y)
        {
            EditorData::Line l;

            draw.attrRect(m_state.x, m_state.y + y, width, 1, m_state.colour);
            l = m_data->getLine(p);

            int l1 = int(l.e1 - l.s1);
            int l2 = int(l.e2 - l.s2);
            if (l.s1)
            {
                int len = min(l1, width);
                draw.printString(m_state.x, m_state.y + y, l.s1, l.s1 + len);

                if (len < width)
                {
                    // Draw 2nd part of line
                    int len2 = min(l2, width - len);
                    draw.printString(m_state.x + len, m_state.y + y, l.s2, l.s2 + len2);

                    if ((len + len2) < width)
                    {
                        draw.clearRect(m_state.x + len, m_state.y + y, (width - len), 1);
                    } 
                }

            }
            else
            {
                // Render empty line
                draw.clearRect(m_state.x, m_state.y + y, width, 1);
            }

            p = l.newPos;
        }
    }
    else
    {
        draw.attrRect(m_state.x, m_state.y, width, height, m_state.colour);
        draw.clearRect(m_state.x, m_state.y, width, height);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// key

bool Editor::key(const KeyEvent& kev)
{
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// text

void Editor::text(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
// setData

void Editor::setData(EditorData& editorData)
{
    m_data = &editorData;
}

//----------------------------------------------------------------------------------------------------------------------
// gotoTop

void Editor::gotoTop()
{
    m_cursor = 0;
    cursorVisible();
}

//----------------------------------------------------------------------------------------------------------------------
// gotoBottom

void Editor::gotoBottom()
{
    assert(m_data);
    m_cursor = m_data->lastPos();
    cursorVisible();
}

//----------------------------------------------------------------------------------------------------------------------
// cursorVisible

void Editor::cursorVisible()
{
    if (!m_data) return;

    i64 currentLine = m_data->getLineNumber(m_cursor);
    i64 botLine = m_top + m_state.height;

    if (currentLine < m_top || currentLine >= botLine)
    {
        // Cursor is outside window.  Bring it back in!
        m_top = __max(0, currentLine - (m_state.height / 2));
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
