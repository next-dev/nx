//----------------------------------------------------------------------------------------------------------------------
// Editor implementation
//----------------------------------------------------------------------------------------------------------------------

#include "editor.h"

//----------------------------------------------------------------------------------------------------------------------
// SplitView
//----------------------------------------------------------------------------------------------------------------------

SplitView::SplitView(const vector<char>& v, size_t start1, size_t end1, size_t start2, size_t end2)
    : m_array(v)
    , m_start { start1, start2 }
    , m_end { end1, end2 }
{

}

SplitView::SplitView(const vector<char>& v, size_t start, size_t end)
    : SplitView(v, start, end, end, end)
{

}

//----------------------------------------------------------------------------------------------------------------------
// EditorData
//----------------------------------------------------------------------------------------------------------------------

EditorData::EditorData(size_t initialSize, size_t increaseSize)
    : m_buffer(initialSize)
    , m_lines(1, 0)
    , m_cursor(0)
    , m_currentLine(0)
    , m_endBuffer(m_buffer.size())
    , m_increaseSize(increaseSize)
{

}

void EditorData::clear()
{
    m_cursor = 0;
    m_endBuffer = m_buffer.size();
}

SplitView EditorData::getLine(int n) const
{
    return SplitView(m_buffer, 0, 0, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------
// Editor
//----------------------------------------------------------------------------------------------------------------------

Editor::Editor(int xCell, int yCell, int width, int height, u8 bkgColour, bool font6, int initialSize, int increaseSize)
    : m_data(initialSize, increaseSize)
    , m_x(xCell)
    , m_y(yCell)
    , m_width(width)
    , m_height(height)
    , m_topLine(0)
    , m_currentLine(0)
    , m_currentX(0)
    , m_font6(font6)
    , m_bkgColour(bkgColour)
{

}

void Editor::render(Draw& draw, int line)
{
    int y = line - m_topLine;
    if (y < m_height)
    {
        // This line is visible
        int x = m_x;
        y += m_y;

        auto view = m_data.getLine((size_t)line);
        for (size_t i = 0; x < m_width; ++x, ++i)
        {
            draw.printChar(x, y, view[i], m_bkgColour);
            if (m_currentX == i) draw.pokeAttr(x, y, m_bkgColour | 0x80);
        }
    }
}

bool Editor::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    return false;
}

void Editor::clear()
{
    m_data.clear();
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
