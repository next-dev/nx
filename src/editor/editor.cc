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
    m_data = new EditorData();
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
    int width = draw.getWidth();
    int height = draw.getHeight();

    if (m_data)
    {
        auto p = m_top;

        for (int y = 0; y < height; ++y)
        {
            EditorData::Line l;

            draw.attrRect(0, y, width, 1, m_state.colour);
            l = m_data->getLine(p);

            int l1 = int(l.e1 - l.s1);
            int l2 = int(l.e2 - l.s2);
            if (l.s1)
            {
                int len = min(l1, width);
                draw.printString(0, y, l.s1, l.s1 + len);

                if (len < width)
                {
                    // Draw 2nd part of line
                    int len2 = min(l2, width - len);
                    draw.printString(len, y, l.s2, l.s2 + len2);

                    if ((len + len2) < width)
                    {
                        draw.clearRect(len, y, (width - len), 1);
                    } 
                }

            }
            else
            {
                // Render empty line
                draw.clearRect(0, y, width, 1);
            }

            p = l.newPos;
        }
    }
    else
    {
        draw.attrRect(0, 0, width, height, m_state.colour);
        draw.clearRect(0, 0, width, height);
    }
}
