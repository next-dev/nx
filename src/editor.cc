//----------------------------------------------------------------------------------------------------------------------
// Editor implementation
//----------------------------------------------------------------------------------------------------------------------

#include "editor.h"

#if NX_DEBUG_EDITOR
#   define DUMP() dump()
#else
#   define DUMP()
#endif

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
    : m_buffer(16)
    , m_lines(1, 0)
    , m_cursor(0)
    , m_currentLine(0)
    , m_endBuffer((int)m_buffer.size())
    , m_increaseSize(increaseSize)
    , m_maxLineLength(maxLineLength)
    , m_lastOffset(-1)
{

}

void EditorData::clear()
{
    m_cursor = 0;
    m_endBuffer = (int)m_buffer.size();
    m_lines.clear();
    m_lines.push_back(0);
    m_currentLine = 0;
}

SplitView EditorData::getLine(int n) const
{
    int nextLinePos = (n + 1 >= (int)m_lines.size()) ? (int)m_buffer.size() : m_lines[n + 1];
    if (m_currentLine == n)
    {
        // m_cursor is between m_lines[n] and nextLinePos.
        return (m_cursor == nextLinePos)
            ? SplitView(m_buffer, m_lines[n], m_cursor, 0, 0)
            : SplitView(m_buffer, m_lines[n], m_cursor, m_endBuffer, nextLinePos);
    }
    else if (n < m_lines.size())
    {
        int i = m_lines[n];
        int start[2] = { 0 }, end[2] = { 0 };
        start[0] = i;
        int idx = 0;
        for (; m_buffer.begin() + i < m_buffer.end() && (i != m_cursor && m_buffer[i] != '\n'); ++i)
        {
            if (i == m_cursor)
            {
                i = m_endBuffer - 1;
                end[idx++] = m_cursor;
                start[idx] = m_endBuffer;
            }
        }
        end[idx] = i;
        return SplitView(m_buffer, start[0], end[0], start[1], end[1]);
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

int EditorData::dataLength() const
{
    return m_cursor + ((int)m_buffer.size() - m_endBuffer);
}

void EditorData::dump() const
{
    NX_LOG("----------------------------------------------------------\n");
    for (int i = 0; i < m_cursor; ++i)
    {
        NX_LOG("%04d: %c\n", i, m_buffer[i]);
    }
    NX_LOG("----\n");
    for (int i = m_endBuffer; i < (int)m_buffer.size(); ++i)
    {
        NX_LOG("%04d: %c\n", i, m_buffer[i]);
    }
    NX_LOG("\n");
    for (int i = 0; i < m_lines.size(); ++i)
    {
        NX_LOG(m_currentLine == i ? "*" : " ");
        NX_LOG("%04d: %d\n", i, m_lines[i]);
    }
}

int EditorData::toVirtualPos(int actualPos) const
{
    return actualPos > m_cursor ? actualPos - m_endBuffer + m_cursor : actualPos;
}

//----------------------------------------------------------------------------------------------------------------------
// Editor axioms
//----------------------------------------------------------------------------------------------------------------------

void EditorData::insert(char ch)
{
    if (lineLength(m_currentLine) == m_maxLineLength) return;
    bool result = ensureSpace(1);
    if (result)
    {
        m_buffer[m_cursor++] = ch;
    }

    m_lastOffset = -1;
    DUMP();
}

void EditorData::moveTo(int pos)
{
    pos = max(0, min(pos, dataLength()));
    if (pos != m_cursor)
    {
        if (pos < m_cursor)
        {
            // Cursor moving left
            // Moving X->Y
            //
            //         <--- l -->
            // +-------+--------+--------------+--------+-----------+
            // |       |XXXXXXXX|              |YYYYYYYY|           |
            // +-------+--------+--------------+--------+-----------+
            //         pos      cursor         newPos   endBuffer
            //

            // Move the X section to the Y section
            int l = m_cursor - pos;
            int newEnd = m_endBuffer - l;
            int oldCursor = m_cursor;
            move(m_buffer.begin() + pos, m_buffer.begin() + m_cursor, m_buffer.begin() + newEnd);
            m_endBuffer = newEnd;
            m_cursor = pos;

            // Any line references between [pos, cursor) need to be adjusted
            int adjust = newEnd - pos;
            for (int i = 0; i < m_lines.size() && m_lines[i] <= oldCursor; ++i)
            {
                if (m_lines[i] > pos)
                {
                    m_lines[i] += adjust;
                }
            }

            while (m_lines[m_currentLine] > m_cursor) --m_currentLine;
        }
        else
        {
            // Cursor moving right
            // Moving X->Y
            //
            //                                   <- l -->
            // +--------------+------+-----------+------+------------+
            // |              |YYYYYY|           |XXXXXX|            |
            // +--------------+------+-----------+------+------------+
            //                ^      newCursor   ^      actualPos
            //                cursor             endBuffer
            //
            int actualPos = m_endBuffer + (pos - m_cursor);
            int adjust = m_endBuffer - m_cursor;
            int origEndBuffer = m_endBuffer;
            int l = actualPos - m_endBuffer;
            move(m_buffer.begin() + m_endBuffer, m_buffer.begin() + actualPos, m_buffer.begin() + m_cursor);
            m_cursor += l;
            m_endBuffer = actualPos;

            // Any line references between [endBuffer, actualPos) need to be adjusted
            for (int i = 0; i < m_lines.size() && m_lines[i] <= actualPos; ++i)
            {
                if (m_lines[i] >= origEndBuffer)
                {
                    m_lines[i] -= adjust;
                }
            }

            while (m_currentLine < int(m_lines.size() - 1) && m_lines[m_currentLine+1] <= m_cursor) ++m_currentLine;
        }
    }

    DUMP();
}

void EditorData::deleteChar(int num)
{
    num = min(num, int(m_buffer.size()) - m_endBuffer);
    for (int i = m_endBuffer; i < m_endBuffer + num; ++i)
    {
        if (m_buffer[i] == '\n')
        {
            m_lines.erase(m_lines.begin() + m_currentLine + 1);
        }
    }
    m_endBuffer += num;
    moveTo(m_cursor);
    m_lastOffset = -1;
}

void EditorData::leftChar(int num = 1)
{
    moveTo(max(0, m_cursor - num));
    m_lastOffset = -1;
}

void EditorData::rightChar(int num = 1)
{
    moveTo(min(dataLength(), m_cursor + num));
    m_lastOffset = -1;
}

void EditorData::upChar(int num)
{
    num = min(num, m_currentLine);
    if (!num) return;
    int newLine = m_currentLine - num;

    m_lastOffset = m_lastOffset == -1 ? getCurrentPosInLine() : m_lastOffset;
    int len = lineLength(newLine);
    int requiredOffset = m_lastOffset > len ? len : m_lastOffset;
    moveTo(toVirtualPos(m_lines[newLine] + requiredOffset));
}

void EditorData::downChar(int num)
{
    num = min(num, int(m_lines.size() - m_currentLine - 1));
    if (!num) return;
    int newLine = m_currentLine + num;

    m_lastOffset = m_lastOffset == -1 ? getCurrentPosInLine() : m_lastOffset;
    int len = lineLength(newLine);
    int requiredOffset = m_lastOffset > len ? len : m_lastOffset;
    moveTo(toVirtualPos(m_lines[newLine] + requiredOffset));
}

void EditorData::backspace()
{
    int lineStart = m_lines[m_currentLine];
    if (m_cursor == lineStart)
    {
        // Delete newline
        if (m_currentLine == 0) return;
        m_lines.erase(m_lines.begin() + m_currentLine);
        --m_currentLine;
    }
    --m_cursor;
    m_lastOffset = -1;
    DUMP();
}

void EditorData::newline()
{
    insert(0x0a);
    m_lines.insert(m_lines.begin() + m_currentLine + 1, m_cursor);
    ++m_currentLine;
    DUMP();
}

//----------------------------------------------------------------------------------------------------------------------
// Queries of state
//----------------------------------------------------------------------------------------------------------------------

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
        m_endBuffer += m_increaseSize;
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
    if (down && !shift && !ctrl && !alt)
    {
        using K = sf::Keyboard::Key;

        switch (key)
        {
        case K::Left:
            m_data.leftChar(1);
            break;

        case K::Right:
            m_data.rightChar(1);
            break;

        case K::Up:
            m_data.upChar(1);
            break;

        case K::Down:
            m_data.downChar(1);
            break;

        case K::Delete:
            m_data.deleteChar(1);
            break;

        default:
            break;
        }
    }
    return true;
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
                data.insert(ch);
                return true;
            }
        }

        switch (ch)
        {
        case 8:     // backspace
            data.backspace();
            break;

        case 13:    // newline
            data.newline();
            break;
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
