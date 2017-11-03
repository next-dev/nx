//----------------------------------------------------------------------------------------------------------------------
// Editor implementation
//----------------------------------------------------------------------------------------------------------------------

#include "editor.h"

//----------------------------------------------------------------------------------------------------------------------
// SplitView
//----------------------------------------------------------------------------------------------------------------------

SplitView::SplitView(const vector<char>& v, int start1, int end1, int start2, int end2)
    : m_array(v)
    , m_start { start1, start2 }
    , m_end { end1, end2 }
{

}

SplitView::SplitView(const vector<char>& v, int start, int end)
    : SplitView(v, start, end, end, end)
{

}

//----------------------------------------------------------------------------------------------------------------------
// EditorData
//----------------------------------------------------------------------------------------------------------------------

EditorData::EditorData(int initialSize, int increaseSize, int maxLineLength)
    : m_buffer(initialSize)
    , m_lines(1, 0)
    , m_cursor(0)
    , m_currentLine(0)
    , m_endBuffer((int)m_buffer.size())
    , m_increaseSize(increaseSize)
    , m_maxLineLength(maxLineLength)
{

}

void EditorData::clear()
{
    m_cursor = 0;
    m_endBuffer = (int)m_buffer.size();
    m_lines.clear();
    m_lines.push_back(0);
}

SplitView EditorData::getLine(int n) const
{
    if (m_currentLine == n)
    {
        return SplitView(m_buffer,
                         m_lines[n], m_cursor,
                         m_endBuffer, (int)m_buffer.size());
    }
    else if (n < m_lines.size())
    {
        int i = m_lines[n];
        for (; m_buffer.begin() + i < m_buffer.end() && m_buffer[i] != '\n'; ++i) ;
        return SplitView(m_buffer, m_lines[n], i);
    }
    else
    {
        return SplitView(m_buffer, 0, 0);
    }
}

SplitView EditorData::getText() const
{
    return SplitView(m_buffer, 0, m_cursor, m_endBuffer, (int)m_buffer.size());
}

int EditorData::lineLength(int n) const
{
    int start = m_lines[n];
    int end;
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
    if (lineLength(m_currentLine) == m_maxLineLength) return false;
    bool result = ensureSpace(1);
    if (result)
    {
        m_buffer[m_cursor++] = ch;
    }
    
    return result;
}

bool EditorData::backspace()
{
    int lineStart = m_lines[m_currentLine];
    if (m_cursor == lineStart)
    {
        // Delete newline
        if (m_currentLine == 0) return false;
        --m_currentLine;
    }
    --m_cursor;
    return true;
}

int EditorData::getCurrentPosInLine() const
{
    return m_cursor - m_lines[m_currentLine];
}

bool EditorData::ensureSpace(int numChars)
{
    if (m_cursor + numChars <= m_endBuffer) return true;
    
    // There is no space
    bool result = false;
    if (m_increaseSize)
    {
        int oldSize = (int)m_buffer.size();
        m_buffer.resize(oldSize + m_increaseSize);
        result = true;
    
        // Move text afterwards forward
        int len = (int)(m_buffer.end() - (m_buffer.begin() + m_endBuffer));
        move(m_buffer.begin() + m_endBuffer, m_buffer.begin() + oldSize, m_buffer.end() - len);
    }
    
    return result;
    
}

//----------------------------------------------------------------------------------------------------------------------
// Editor
//----------------------------------------------------------------------------------------------------------------------

Editor::Editor(int xCell, int yCell, int width, int height, u8 bkgColour, bool font6, int initialSize, int increaseSize)
    : m_data(initialSize, increaseSize, width-1)
    , m_x(xCell)
    , m_y(yCell)
    , m_width(width)
    , m_height(height)
    , m_topLine(0)
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
    return getData().getText();
}

void Editor::render(Draw& draw, int line)
{
    EditorData& data = getData();
    int y = line - m_topLine;
    if (y < m_height)
    {
        // This line is visible
        int x = m_x;
        y += m_y;

        auto view = data.getLine((int)line);
        for (int i = 0; x < m_x + m_width; ++x, ++i)
        {
            draw.printChar(x, y, view[i], m_bkgColour);
        }

        int currentX = m_data.getCurrentPosInLine();
        if ((m_data.getCurrentLine() == line) && (m_data.getCurrentPosInLine() < m_width))
        {
            draw.pokeAttr(m_x + currentX, y, Draw::attr(Colour::White, Colour::Blue, true) | 0x80);
        }
    }
}

void Editor::renderAll(Draw& draw)
{
    int line = m_topLine;
    int y = m_y;
    int endY = m_y + m_height;

    for (; y < endY; ++y)
    {
        render(draw, line++);
    }
}

bool Editor::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    return false;
}

bool Editor::text(char ch)
{
    EditorData& data = getData();
    if (m_allowedChars[ch])
    {
        if (m_data.getCurrentPosInLine() < m_width)
        {
            if (ch >= ' ' && ch < 127)
            {
                return data.insert(ch);
            }
        }

        if (ch == 8)
        {
            // Backspace
            data.backspace();
        }
    }
    return false;
}

void Editor::clear()
{
    getData().clear();
}

//----------------------------------------------------------------------------------------------------------------------
// EditorWindow
//----------------------------------------------------------------------------------------------------------------------

EditorWindow::EditorWindow(Nx& nx, string title)
    : Window(nx, 1, 1, 78, 60, title, Colour::Black, Colour::White, false)
    , m_editor(2, 2, 76, 58, Draw::attr(Colour::Black, Colour::White, false), false, 1024, 1024)
{

}

void EditorWindow::onDraw(Draw& draw)
{
    m_editor.renderAll(draw);
}

void EditorWindow::onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt)
{
    m_editor.key(key, true, shift, ctrl, alt);
}

void EditorWindow::onText(char ch)
{
    m_editor.text(ch);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
