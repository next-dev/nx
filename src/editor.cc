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
    m_lines.clear();
    m_lines.push_back(0);
}

SplitView EditorData::getLine(int n) const
{
    if (m_currentLine == n)
    {
        return SplitView(m_buffer,
                         m_lines[n], m_cursor,
                         m_endBuffer, m_buffer.size());
    }
    else
    {
        size_t i = m_lines[n];
        for (; m_buffer.begin() + i < m_buffer.end() && m_buffer[i] != '\n'; ++i) ;
        return SplitView(m_buffer, m_lines[n], i);
    }
}

SplitView EditorData::getText() const
{
    return SplitView(m_buffer, 0, m_cursor, m_endBuffer, m_buffer.size());
}

size_t EditorData::lineLength(int n) const
{
    size_t start = m_lines[n];
    size_t end;
    if (n == m_currentLine)
    {
        end = m_endBuffer;
        start = m_endBuffer - (m_cursor - start);
    }
    else
    {
        end = start;
    }
    for (; m_buffer.begin() + end < m_buffer.end() && m_buffer[end] != '\n'; ++end) ;
    return end - start;
}

bool EditorData::insert(char ch)
{
    bool result = ensureSpace(1);
    if (result)
    {
        m_buffer[m_cursor++] = ch;
    }
    
    return result;
}

bool EditorData::backspace()
{
    size_t lineStart = m_lines[m_currentLine];
    if (m_cursor == lineStart)
    {
        // Delete newline
        if (m_currentLine == 0) return false;
        --m_currentLine;
    }
    --m_cursor;
    return true;
}

bool EditorData::ensureSpace(size_t numChars)
{
    if (m_cursor + numChars <= m_endBuffer) return true;
    
    // There is no space
    bool result = false;
    if (m_increaseSize)
    {
        size_t oldSize = m_buffer.size();
        m_buffer.resize(oldSize + m_increaseSize);
        result = true;
    
        // Move text afterwards forward
        size_t len = (size_t)(m_buffer.end() - (m_buffer.begin() + m_endBuffer));
        move(m_buffer.begin() + m_endBuffer, m_buffer.begin() + oldSize, m_buffer.end() - len);
    }
    
    return result;
    
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
    , m_allowedChars(128, true)
{
    for (int i = 0; i < 32; ++i) m_allowedChars[i] = false;
    m_allowedChars[127] = false;
    m_allowedChars[8] = true;
    m_allowedChars[13] = true;
}

void Editor::onlyAllowDecimal()
{
    fill(m_allowedChars.begin(), m_allowedChars.end(), false);
    m_allowedChars[8] = true;
    m_allowedChars[13] = true;
    for (int i = '0'; i <= '9'; ++i) m_allowedChars[i] = true;
}

void Editor::onlyAllowHex()
{
    onlyAllowDecimal();
    for (int i = 'a'; i <= 'f'; ++i) m_allowedChars[i] = true;
    for (int i = 'A'; i <= 'F'; ++i) m_allowedChars[i] = true;
}

SplitView Editor::getText() const
{
    return m_data.getText();
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

bool Editor::text(char ch)
{
    if (m_allowedChars[ch])
    {
        if (m_currentX < m_width)
        {
            if (ch >= ' ' && ch < 127)
            {
                if (m_data.insert(ch)) ++m_currentX;
                return true;
            }
            else if (ch == 8)
            {
                // Backspace
                if (m_data.backspace())
                {
                    if (--m_currentX < 0)
                    {
                        m_currentX = (int)m_data.lineLength(--m_currentLine);
                    }
                }
            }
        }
    }
    return false;
}

void Editor::clear()
{
    m_data.clear();
    m_currentX = 0;
    m_currentLine = 0;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
