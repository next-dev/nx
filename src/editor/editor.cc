//----------------------------------------------------------------------------------------------------------------------
// Editor implementation
//----------------------------------------------------------------------------------------------------------------------

#include <editor/editor.h>
#include <utils/format.h>
#include <utils/tinyfiledialogs.h>

#include <algorithm>
#include <fstream>

#if NX_DEBUG_EDITOR
#   define DUMP() dump()
#else
#   define DUMP()
#endif

#define K_LINE_SKIP 20

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

EditorData::EditorData(int initialSize, int increaseSize)
    : m_buffer(16)
    , m_lines(1, 0)
    , m_cursor(0)
    , m_currentLine(0)
    , m_endBuffer((int)m_buffer.size())
    , m_increaseSize(increaseSize)
    , m_lastOffset(-1)
    , m_changed(false)
    , m_initialTabs()
    , m_tabSize(1)
{

}

void EditorData::clear()
{
    m_cursor = 0;
    m_endBuffer = (int)m_buffer.size();
    m_lines.clear();
    m_lines.push_back(0);
    m_currentLine = 0;
    resetChanged();
}

SplitView EditorData::getLine(int n) const
{
    DataPos nextLinePos = (n + 1 >= (int)m_lines.size()) ? (int)m_buffer.size() : getLinePos(n + 1);
    if (m_currentLine == n)
    {
        // m_cursor is between m_lines[n] and nextLinePos.
        return (m_cursor == nextLinePos)
            ? SplitView(m_buffer, (int)getLinePos(n), (int)m_cursor, 0, 0)
            : SplitView(m_buffer, (int)getLinePos(n), (int)m_cursor, (int)m_endBuffer, (int)nextLinePos);
    }
    else if (n < m_lines.size())
    {
        DataPos i = getLinePos(n);
        int start[2] = { 0 }, end[2] = { 0 };
        start[0] = (int)i;
        int idx = 0;
        for (; (m_buffer.begin() + (int)i) < m_buffer.end() && (i != m_cursor && getChar(i) != '\n'); ++i)
        {
            if (i == m_cursor)
            {
                i = m_endBuffer - 1;
                end[idx++] = (int)m_cursor;
                start[idx] = (int)m_endBuffer;
            }
        }
        end[idx] = (int)i;
        return SplitView(m_buffer, start[0], end[0], start[1], end[1]);
    }
    else
    {
        return SplitView(m_buffer, 0, 0);
    }
}

SplitView EditorData::getText() const
{
    return SplitView(m_buffer, 0, (int)m_cursor, (int)m_endBuffer, (int)m_buffer.size());
}

vector<u8> EditorData::getData() const
{
    vector<u8> data(m_buffer.begin(), m_buffer.begin() + (int)m_cursor);
    data.insert(data.end(), m_buffer.begin() + (int)m_endBuffer, m_buffer.end());
    return data;
}

int EditorData::lineLength(int n) const
{
    DataPos start = getLinePos(n);
    DataPos end;
    if (n == m_currentLine)
    {
        end = m_endBuffer;
        start = m_endBuffer - (m_cursor - start);
    }
    else
    {
        end = start;
    }
    for (; m_buffer.begin() + (int)end < m_buffer.end() && getChar(end) != '\n'; ++end) ;
    return end - start;
}

int EditorData::dataLength() const
{
    return (int)m_cursor + ((int)m_buffer.size() - (int)m_endBuffer);
}

int EditorData::getNumLines() const
{
    return int(m_lines.size());
}

void EditorData::dump() const
{
    NX_LOG("----------------------------------------------------------\n");
    for (DataPos i = 0; i < m_cursor; ++i)
    {
        NX_LOG("%04d: %c\n", i, getChar(i));
    }
    NX_LOG("----\n");
    for (DataPos i = m_endBuffer; i < (int)m_buffer.size(); ++i)
    {
        NX_LOG("%04d: %c\n", i, getChar(i));
    }
    NX_LOG("\n");
    for (int i = 0; i < m_lines.size(); ++i)
    {
        NX_LOG(m_currentLine == i ? "*" : " ");
        NX_LOG("%04d: %d\n", i, getLinePos(i));
    }
}

bool EditorData::isValidPos(Pos pos) const
{
    return
        (pos >= 0) &&
        (pos <= (int)m_cursor + ((int)m_buffer.size() - (int)m_endBuffer));
}

bool EditorData::isValidPos(DataPos pos) const
{
    return
        (pos >= 0 && pos <= m_cursor) ||
        (pos >= m_endBuffer && pos <= (int)m_buffer.size());
}

EditorData::Pos EditorData::toVirtualPos(DataPos actualPos) const
{
    assert(isValidPos(actualPos));
    return (int)(actualPos > m_cursor ? m_cursor + (actualPos - m_endBuffer) : actualPos);
}

EditorData::DataPos EditorData::toDataPos(Pos virtualPos) const
{
    assert(isValidPos(virtualPos));
    return (int)(virtualPos > int(m_cursor) ? m_endBuffer + (DataPos((int)virtualPos) - m_cursor) : (int)virtualPos);
}

//----------------------------------------------------------------------------------------------------------------------
// Editor axioms
//----------------------------------------------------------------------------------------------------------------------

void EditorData::insert(char ch)
{
    bool result = ensureSpace(1);
    if (result)
    {
        setChar(m_cursor++, ch);
    }

    m_lastOffset = -1;
    changed();
    DUMP();
}

void EditorData::insert(string str)
{
    for (auto ch : str)
    {
        insert(ch);
    }
}

void EditorData::moveTo(Pos pos)
{
    Pos p = max(0, min((int)pos, dataLength()));
    DataPos dp = toDataPos(p);
    Pos cursor = toVirtualPos(m_cursor);
    if (p != cursor)
    {
        if (p < cursor)
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
            int l = cursor - p;
            DataPos newEnd = m_endBuffer - l;
            DataPos oldCursor = m_cursor;
            move(m_buffer.begin() + (int)p, m_buffer.begin() + (int)m_cursor, m_buffer.begin() + (int)newEnd);
            m_endBuffer = newEnd;
            m_cursor = dp;

            // Any line references between [pos, cursor) need to be adjusted
            int adjust = newEnd - m_cursor;
            for (int i = 0; i < m_lines.size() && getLinePos(i) <= oldCursor; ++i)
            {
                if (getLinePos(i) > dp)
                {
                    setLinePos(i, getLinePos(i) + adjust);
                }
            }

            while (getLinePos(m_currentLine) > m_cursor) --m_currentLine;
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
            int adjust = m_endBuffer - m_cursor;
            DataPos origEndBuffer = m_endBuffer;
            int l = dp - m_endBuffer;
            move(m_buffer.begin() + (int)m_endBuffer, m_buffer.begin() + (int)dp, m_buffer.begin() + (int)m_cursor);
            m_cursor += l;
            m_endBuffer = dp;

            // Any line references between [endBuffer, dp) need to be adjusted
            for (int i = 0; i < m_lines.size() && getLinePos(i) <= dp; ++i)
            {
                if (getLinePos(i) >= origEndBuffer)
                {
                    setLinePos(i, getLinePos(i) - adjust);
                }
            }

            while (m_currentLine < int(m_lines.size() - 1) && getLinePos(m_currentLine+1) <= m_cursor) ++m_currentLine;
        }
    }

    DUMP();
}

void EditorData::deleteChar(int num)
{
    num = min(num, int(m_buffer.size()) - (int)m_endBuffer);
    for (DataPos i = m_endBuffer; i < m_endBuffer + num; ++i)
    {
        if (getChar(i) == '\n')
        {
            m_lines.erase(m_lines.begin() + m_currentLine + 1);
        }
    }
    m_endBuffer += num;
    moveTo(Pos((int)m_cursor));
    m_lastOffset = -1;
    changed();
}

void EditorData::leftChar(int num = 1)
{
    moveTo(max(0, (int)m_cursor - num));
    m_lastOffset = -1;
}

void EditorData::rightChar(int num = 1)
{
    moveTo(min(dataLength(), (int)m_cursor + num));
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
    moveTo(toVirtualPos(getLinePos(newLine) + requiredOffset));
}

void EditorData::downChar(int num)
{
    num = min(num, int(m_lines.size() - m_currentLine - 1));
    if (!num) return;
    int newLine = m_currentLine + num;

    m_lastOffset = m_lastOffset == -1 ? getCurrentPosInLine() : m_lastOffset;
    int len = lineLength(newLine);
    int requiredOffset = m_lastOffset > len ? len : m_lastOffset;
    moveTo(toVirtualPos(getLinePos(newLine) + requiredOffset));
}

void EditorData::backspace(int num)
{
    // Find out if it's all spaces to the previous tab position (but only if we delete 1 character)
    int count = 1;
    if (num == 1)
    {
        DataPos p1 = getLinePos(getCurrentLine()) + lastTabPos();
        DataPos p2 = m_cursor - 1;
        while (p2 > p1)
        {
            if (getChar(--p2) != ' ')
            {
                // At least one of the characters after the previous tab is a non-space.
                // So just delete one character.
                ++p2;   // So p1 != p2 at end of this loop
                break;
            }
        }
        if (p2 == p1) count = m_cursor - p1;
    }
    else
    {
        count = num;
    }

    for (int i = 0; i < count; ++i)
    {
        DataPos lineStart = getLinePos(m_currentLine);
        if (m_cursor == (int)lineStart)
        {
            // Delete newline
            if (m_currentLine == 0) return;
            m_lines.erase(m_lines.begin() + m_currentLine);
            --m_currentLine;
        }
        --m_cursor;
    }
    m_lastOffset = -1;
    DUMP();
    changed();
}

void EditorData::newline(bool indentFlag)
{
    int indent = 0;

    // Indent to the previous line's position
    if (indentFlag)
    {
        int line = getCurrentLine();
        DataPos p = getLinePos(line);
        while (getChar(p++) == ' ')
        {
            ++indent;
        }
    }

    // Remove any spaces before the newline
    while (m_cursor > 0 && m_buffer[(int)m_cursor - 1] == ' ') leftChar(1);

    // Insert a new line
    insert(0x0a);
    m_lines.insert(m_lines.begin() + m_currentLine + 1, m_cursor);
    ++m_currentLine;
    DUMP();
    changed();

    // Indent
    for (int i = 0; i < indent; ++i) insert(' ');
}

void EditorData::home()
{
    moveTo(toVirtualPos(getLinePos(m_currentLine)));
    m_lastOffset = -1;
}

void EditorData::end()
{
    if (m_currentLine == m_lines.size() - 1)
    {
        // Last line
        moveTo(dataLength());
    }
    else
    {
        moveTo((int)toVirtualPos(getLinePos(m_currentLine+1))-1);
    }
    m_lastOffset = -1;
}

void EditorData::cutLine()
{
    copyLine();
    home();
    deleteChar(lineLength(m_currentLine) + 1);
}

void EditorData::copyLine()
{
    Pos linePos = getPosAtLine(m_currentLine);
    Pos endPos = linePos + lineLength(m_currentLine);

    DataPos p0 = toDataPos(linePos);
    DataPos p1 = toDataPos(endPos);

    m_clipboard.clear();

    int a0 = (int)min(p0, m_cursor);
    int a1 = (int)min(p1, m_cursor);
    int b0 = (int)max(p0, m_endBuffer);
    int b1 = (int)max(p1, m_endBuffer);

    m_clipboard.insert(m_clipboard.end(), m_buffer.begin() + a0, m_buffer.begin() + a1);
    m_clipboard.insert(m_clipboard.end(), m_buffer.begin() + b0, m_buffer.begin() + b1);
}

void EditorData::pasteLine()
{
    home();
    insert(string(m_clipboard.begin(), m_clipboard.end()));
    newline(false);
}

//----------------------------------------------------------------------------------------------------------------------
// Tabs
//----------------------------------------------------------------------------------------------------------------------

void EditorData::tab()
{
    int x = getCurrentPosInLine();
    int tab = x;

    for (int i = 0; i < m_initialTabs.size(); ++i)
    {
        if (x < m_initialTabs[i])
        {
            // Found the next tab
            tab = m_initialTabs[i];
            break;
        }
    }
    if (tab == x)
    {
        // Couldn't find a tab.  Use normal tabs
        tab = x + (m_tabSize - (x % m_tabSize));
    }

    while (x < tab)
    {
        insert(' ');
        ++x;
    }
}

int EditorData::lastTabPos() const
{
    // Calculate last tab position
    int x = getCurrentPosInLine();
    int tab = 0;

    if (!m_initialTabs.empty() && x > m_initialTabs.back())
    {
        // We're in normal tabs territory.
        tab = m_initialTabs.back() +
            ((((x - m_initialTabs.back()) - 1) / m_tabSize) * m_tabSize);
    }
    else
    {
        for (int i = (int)m_initialTabs.size() - 1; i >= 0; --i)
        {
            if (x > m_initialTabs[i])
            {
                tab = m_initialTabs[i];
                break;
            }
        }
    }

    return tab;
}

void EditorData::untab()
{
    int tab = lastTabPos();
    int x = getCurrentPosInLine();

    // tab points to the place we should go.  Keep deleting spaces until we reach a non-space or the tab position
    while (x > tab)
    {
        if (getChar(m_cursor - 1) != ' ') break;
        backspace(1);
        --x;
    }
}

void EditorData::setTabs(vector<int> tabs, int tabSize)
{
    m_initialTabs = tabs;
    m_tabSize = tabSize;
}

//----------------------------------------------------------------------------------------------------------------------
// Queries of state
//----------------------------------------------------------------------------------------------------------------------

int EditorData::getCurrentPosInLine() const
{
    return m_cursor - getLinePos(m_currentLine);
}

EditorData::Pos EditorData::getPosAtLine(int line) const
{
    return toVirtualPos(getLinePos(line));
}

bool EditorData::ensureSpace(int numChars)
{
    if (m_cursor + numChars <= m_endBuffer) return true;
    
    // There is no space
    bool result = false;
    if (m_increaseSize)
    {
        int delta = (numChars / m_increaseSize + 1) * m_increaseSize;
        int oldSize = (int)m_buffer.size();
        m_buffer.resize(oldSize + delta);
        result = true;
    
        // Move text afterwards forward
        int len = (int)(m_buffer.end() - (m_buffer.begin() + (int)m_endBuffer));
        move(m_buffer.begin() + (int)m_endBuffer, m_buffer.begin() + oldSize, m_buffer.end() - len);
        m_endBuffer += delta;
    }
    
    return result;
}

int EditorData::getCharCategory(char ch)
{
    if (ch <= ' ' || ch > 127) return 0;
    if (ch >= 'a' && ch <= 'z') return 1;
    if (ch >= 'A' && ch <= 'Z') return 1;
    if (ch >= '0' && ch <= '9') return 1;
    if (ch == '_') return 1;
    return 2;
}

EditorData::Pos EditorData::lastWordPos() const
{
    if (m_cursor == 0) return 0;

    DataPos x = m_cursor - 1;

    // Skip past any space (char category == 0)
    while (x > 0 && getCharCategory(getChar(x)) == 0)
    {
        --x;
        // Stop at the beginning of each line
        if (getChar(x) == '\n') return toVirtualPos(x + 1);
    }

    // Find the beginning of the group of characters that share the same category
    int origCat = getCharCategory(getChar(x));
    while (x > 0 && getCharCategory(getChar(x-1)) == origCat)
    {
        --x;
    }

    return toVirtualPos(x);
}

EditorData::Pos EditorData::nextWordPos() const
{
    DataPos x = m_endBuffer;
    if (x == (int)m_buffer.size()) return toVirtualPos(x);

    // Skip past any characters of the same category
    int origCat = getCharCategory(getChar(x));
    while (x < (int)m_buffer.size() && getCharCategory(getChar(x)) == origCat)
    {
        ++x;
    }
    if (x == (int)m_buffer.size() || getChar(x) == '\n') return toVirtualPos(x);

    // Skip past any space (char catgory == 0)
    while (x < (int)m_buffer.size() && getCharCategory(getChar(x)) == 0)
    {
        ++x;
        if (getChar(x) == '\n') return toVirtualPos(x);
    }

    return toVirtualPos(x);
}

//----------------------------------------------------------------------------------------------------------------------
// File access
//----------------------------------------------------------------------------------------------------------------------

bool EditorData::load(const char* fileName)
{
    clear();

    ifstream f;
    f.open(fileName, ios::binary | ios::in);
    if (f.is_open())
    {
        f.seekg(0, ios::end);
        int size = (int)f.tellg();
        f.seekg(0, ios::beg);
        ensureSpace(size);
        char* buffer = (char *)m_buffer.data();
        f.read(buffer, size);
        f.close();

        // Update pointers
        m_cursor = size;

        // Update lines
        for (DataPos i = 0; i < m_cursor; ++i)
        {
            if (getChar(i) == 0x0a)
            {
                m_lines.emplace_back(i+1);
            }
        }

        m_currentLine = (int)m_lines.size() - 1;
        moveTo(0);

        return true;
    }
    else
    {
        return false;
    }
}

bool EditorData::save(const char* fileName)
{
    ofstream f;
    f.open(fileName, ios::binary | ios::trunc | ios::out);
    if (f.is_open())
    {
        f.write(m_buffer.data(), (size_t)(int)m_cursor);
        f.write(m_buffer.data() + (int)m_endBuffer, m_buffer.size() - (int)m_endBuffer);
        f.close();
        resetChanged();
        return true;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Editor
//----------------------------------------------------------------------------------------------------------------------

Editor::Editor(int xCell,
               int yCell,
               int width,
               int height,
               u8 bkgColour,
               bool font6,
               int initialSize,
               int increaseSize,
               EnterFunction onEnter)
    : m_data(initialSize, increaseSize)
    , m_x(xCell)
    , m_y(yCell)
    , m_width(width)
    , m_height(height)
    , m_topLine(0)
    , m_lineOffset(0)
    , m_font6(font6)
    , m_bkgColour(bkgColour)
    , m_commentColour(bkgColour)
    , m_allowedChars(128, true)
    , m_ioAllowed(increaseSize != 0)
    , m_onEnter(onEnter)
    , m_showLineNumbers(false)
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

void Editor::setCommentColour(u8 colour)
{
    m_commentColour = colour;
}

void Editor::setLineNumberColour(u8 colour)
{
    m_lineNumberColour = colour;
}

SplitView Editor::getText() const
{
    return getData().getText();
}

string Editor::getTitle() const
{
    string title = getFileName();
    if (title.empty()) title = "[new file]";
    if (m_data.hasChanged()) title += "*";
    return title;
}

void Editor::render(Draw& draw, int line, int lineNumberGap)
{
    EditorData& data = getData();
    u8 colour = m_bkgColour;
    int y = line - m_topLine;
    if (y < m_height)
    {
        // This line is visible
        int x = m_x + lineNumberGap;
        y += m_y;

        if (lineNumberGap && line < getData().getNumLines())
        {
            draw.printSquashedString(m_x, y, intString(line + 1, 0), m_lineNumberColour);
        }

        auto view = data.getLine((int)line);

        // Text off-screen is important for setting the syntax 
        for (int i = 0; i < m_lineOffset; ++i)
        {
            if (view[i] == ';') colour = m_commentColour;
        }
        for (int i = m_lineOffset; x < m_x + m_width; ++x, ++i)
        {
            if (view[i] == ';') colour = m_commentColour;
            draw.printChar(x, y, view[i], colour);
        }

        // Render the cursor if on the same line
        int currentX = m_data.getCurrentPosInLine();
        if ((m_data.getCurrentLine() == line) && (currentX < (m_lineOffset + m_width - lineNumberGap)))
        {
            draw.pokeAttr(m_x + currentX - m_lineOffset + lineNumberGap, y, Draw::attr(Colour::White, Colour::Blue, true) | 0x80);
        }
    }
}

void Editor::renderAll(Draw& draw)
{
    int lineNumberGap = 0;
    if (m_showLineNumbers)
    {
        int width = (int)log((float)getData().getNumLines());
        string maxInteger = string(width, '9');
        int cellWidth = draw.squashedStringWidth(maxInteger);
        lineNumberGap = cellWidth;
    }

    int line = m_topLine;
    int y = m_y;
    int endY = y + m_height;

    for (; y < endY; ++y)
    {
        render(draw, line++, lineNumberGap);
    }
}

void Editor::ensureVisibleCursor()
{
    // Check for up scroll
    if (m_data.getCurrentLine() < m_topLine)
    {
        m_topLine = std::max(0, m_topLine - K_LINE_SKIP);

        // Check to see if visible yet.  If not, just jump.
        if (m_data.getCurrentLine() < m_topLine)
        {
            m_topLine = std::max(0, m_data.getCurrentLine() - (m_height / 2));
        }
    }
    // Check for down scroll
    else if (m_data.getCurrentLine() >= (m_topLine + m_height))
    {
        m_topLine = std::min(m_data.getNumLines() - 1, m_topLine + K_LINE_SKIP);

        // Check to see if visible yet.  If not, just jump.
        if (m_data.getCurrentLine() >= (m_topLine + m_height))
        {
            m_topLine = std::max(0, m_data.getCurrentLine() - (m_height / 2));
        }
    }

    // Check for left scroll
    while(1)
    {
        int x = m_data.getCurrentPosInLine();
        if (x < m_lineOffset)
        {
            m_lineOffset = std::max(0, m_lineOffset - K_LINE_SKIP);
            continue;
        }
        else if (x >= (m_lineOffset + m_width))
        {
            m_lineOffset = std::min(m_data.lineLength(m_data.getCurrentLine()) - 1, m_lineOffset + K_LINE_SKIP);
            continue;
        }
        break;
    }
}

void Editor::save(string fileName)
{
    if (fileName.empty())
    {
        const char* filters[] = { "*.asm", "*.s" };
        const char* fn = tinyfd_saveFileDialog("Save source code", 0, sizeof(filters) / sizeof(filters[0]),
            filters, "Source code");
        fileName = fn ? fn : "";
    }
    if (!fileName.empty())
    {
        // If there is no '.' after the last '\', if there is no '.' and no '\\', we have no extension.
        // Add .asm in this case.
        std::string finalName = fileName;
        char* slashPos = strrchr((char *)fileName.c_str(), '\\');
        char* dotPos = strrchr((char *)fileName.c_str(), '.');
        if ((slashPos && !dotPos) ||
            (slashPos && (dotPos < slashPos)) ||
            (!slashPos && !dotPos))
        {
            finalName += ".asm";
        }

        if (!m_data.save(finalName.c_str()))
        {
            tinyfd_messageBox("ERROR", "Unable to open file!", "ok", "warning", 0);
        }
        else
        {
            setFileName(finalName);
        }
    }

}

bool Editor::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    if (!down) return true;

    //------------------------------------------------------------------------------------------------------------------
    // No shift keys
    //------------------------------------------------------------------------------------------------------------------
    
    if (!shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::Left:
            m_data.leftChar(1);
            ensureVisibleCursor();
            break;

        case K::Right:
            m_data.rightChar(1);
            ensureVisibleCursor();
            break;

        case K::Up:
            m_data.upChar(1);
            ensureVisibleCursor();
            break;

        case K::Down:
            m_data.downChar(1);
            ensureVisibleCursor();
            break;

        case K::Delete:
            m_data.deleteChar(1);
            ensureVisibleCursor();
            break;

        case K::Home:
            m_data.home();
            ensureVisibleCursor();
            break;

        case K::End:
            m_data.end();
            ensureVisibleCursor();
            break;

        case K::PageUp:
            {
                m_data.upChar(m_height);
                m_data.home();
                ensureVisibleCursor();
            }
            break;

        case K::PageDown:
            {
                m_data.downChar(m_height);
                m_data.home();
                ensureVisibleCursor();
            }
            break;

        case K::Tab:
            m_data.tab();
            ensureVisibleCursor();
            break;

        default:
            break;
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    // Shift keys
    //------------------------------------------------------------------------------------------------------------------

    else if (shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::Tab:    // Back tab
            m_data.untab();
            ensureVisibleCursor();
            break;

        default:
            break;
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    // Ctrl-keys
    //------------------------------------------------------------------------------------------------------------------

    else if (!shift && ctrl && !alt)
    {
        switch(key)
        {
        case K::Home:
            m_data.moveTo(0);
            ensureVisibleCursor();
            break;

        case K::End:
            m_data.moveTo(m_data.dataLength());
            ensureVisibleCursor();
            break;

        case K::S:  // Save
            if (m_ioAllowed)
            {
                save(getFileName());
            }
            break;

        case K::Left:   // Ctrl-Left
            m_data.moveTo(m_data.lastWordPos());
            ensureVisibleCursor();
            break;

        case K::Right:  // Ctrl-Right
            m_data.moveTo(m_data.nextWordPos());
            ensureVisibleCursor();
            break;

        case K::C:      // Copy
            m_data.copyLine();
            break;

        case K::X:      // Cut
            m_data.cutLine();
            break;

        case K::V:      // Paste
            m_data.pasteLine();
            break;

        default:
            break;
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    // Shift-Ctrl keys
    //------------------------------------------------------------------------------------------------------------------
    
    else if (ctrl && shift && !alt)
    {
        switch (key)
        {
        case K::S:  // Save As...
            if (m_ioAllowed)
            {
                save(string());
            }
            break;

        case K::L:  // Toggle line numbers
            m_showLineNumbers = !m_showLineNumbers;
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
        if (ch >= ' ' && ch < 127)
        {
            data.insert(ch);
            ensureVisibleCursor();
            return true;
        }

        switch (ch)
        {
        case 8:     // backspace
            data.backspace(1);
            ensureVisibleCursor();
            break;

        case 13:    // newline
            if (m_onEnter)
            {
                m_onEnter(*this);
                ensureVisibleCursor();
            }
            else
            {
                data.newline();
                ensureVisibleCursor();
            }
            break;
        }
    }
    return false;
}

void Editor::clear()
{
    getData().clear();
}

void Editor::setPosition(int xCell, int yCell, int width, int height)
{
    m_x = xCell;
    m_y = yCell;
    m_width = width;
    m_height = height;
}

//----------------------------------------------------------------------------------------------------------------------
// EditorWindow
//----------------------------------------------------------------------------------------------------------------------

EditorWindow::EditorWindow(Nx& nx, string title)
    : Window(nx, 1, 1, 78, 59, title, Colour::Blue, Colour::Black, false)
    , m_editors()
    , m_selectedTab(-1)
    , m_status("Line: {6l}, Column: {6c}")
    , m_statusColour(Draw::attr(Colour::White, Colour::Blue, true))
    , m_currentError(-1)
{
}

void EditorWindow::setDefaultStatus()
{
    setStatus("Line: {6l}, Column: {6c}", Draw::attr(Colour::White, Colour::Blue, true));
}

void EditorWindow::onDraw(Draw& draw)
{
    if (m_editors.empty())
    {
        string line1 = "Press {Ctrl-O} to open a file for editing";
        string line2 = "Press {Ctrl-N} to create a new file";
        u8 colour = draw.attr(Colour::White, Colour::Black, false);

        int y = m_y + (m_height / 2);

        draw.printString(m_x + (m_width - int(line1.size())) / 2, y - 1, line1, true, colour);
        draw.printString(m_x + (m_width - int(line2.size())) / 2, y + 1, line2, true, colour);
    }
    else
    {
        getEditor().renderAll(draw);

        //
        // If Ctrl+Tab is pressed draw the menu
        //
        if (m_selectedTab >= 0)
        {
            int maxWidth = 0;
            for (size_t i = 0; i < m_editorOrder.size(); ++i)
            {
                Editor& editor = m_editors[m_editorOrder[i]];
                int width = draw.squashedStringWidth(editor.getTitle());
                maxWidth = std::max(width + 2, maxWidth);
            }

            maxWidth = std::max(20, maxWidth);
            draw.window(m_x + 1, m_y + 1, maxWidth + 2, int(m_editorOrder.size() + 2), "Buffers", true);

            for (size_t i = 0; i < m_editorOrder.size(); ++i)
            {
                u8 colour = i == m_selectedTab ? Draw::attr(Colour::White, Colour::Red, true) : Draw::attr(Colour::Black, Colour::White, true);
                Editor& editor = m_editors[m_editorOrder[i]];
                for (int x = 0; x < maxWidth; ++x)
                {
                    draw.printChar(m_x + 2 + x, m_y + 2 + int(i), ' ', colour);
                }
                draw.printSquashedString(m_x + 2, m_y + 2 + int(i), editor.getTitle(), colour);
            }
        }
    }

    // Draw status bar
    draw.attrRect(m_x + 0, m_y + m_height, m_width, 1, m_statusColour);

    if (!m_editors.empty())
    {
        string line;
        for (const char* str = m_status.data(); *str != 0; ++str)
        {
            if (*str == '{')
            {
                // Special formatting
                int pad = 0;
                ++str;
                while (*str != '}')
                {
                    if (*str >= '0' && *str <= '9')
                    {
                        pad = (pad * 10) + (*str - '0');
                    }
                    else switch (*str)
                    {
                    case 'l':
                        // Line number
                    {
                        string x = intString(getEditor().getData().getCurrentLine() + 1, pad);
                        line += x;
                        pad = 0;
                    }
                    break;

                    case 'c':
                        // Column number
                    {
                        string x = intString(getEditor().getData().getCurrentPosInLine() + 1, pad);
                        line += x;
                        pad = 0;
                    }
                    break;

                    default:
                        break;
                    }
                    ++str;
                }
            }
            else
            {
                line += *str;
            }
        }
        draw.printSquashedString(m_x + 1, m_y + m_height, line, m_statusColour);
    }
}

void EditorWindow::newFile()
{
    int index = int(m_editors.size());
    m_editors.emplace_back(2, 2, 76, 57, Draw::attr(Colour::White, Colour::Black, false), false, 1024, 1024);
    m_editors.back().setCommentColour(Draw::attr(Colour::Green, Colour::Black, false));
    m_editors.back().setLineNumberColour(Draw::attr(Colour::Cyan, Colour::Black, false));
    m_editors.back().getData().setTabs({ 8, 14, 32 }, 4);
    m_editorOrder.insert(m_editorOrder.begin(), index);

#if NX_DEBUG_CONSOLE
    cout << "NEW -------------------------------\n\n";
    for (const auto& editor : m_editors)
    {
        cout << (editor.getFileName().empty() ? "[new file]" : editor.getFileName()) << endl;
    }
    cout << endl;
#endif
}

void EditorWindow::closeFile()
{
    if (m_editors.empty()) return;

    if (getEditor().getData().hasChanged())
    {
        // Check to see if the user really wants to overwrite their changes.
        if (!tinyfd_messageBox("Are you sure?", "There has been changes since you last saved.  Are you sure you want to lose your changes?",
            "yesno", "question", 0))
        {
            return;
        }
    }

    int index = m_editorOrder[0];
    m_editors.erase(m_editors.begin() + index);
    m_editorOrder.erase(m_editorOrder.begin());
    for (auto& order : m_editorOrder)
    {
        if (order > index) order -= 1;
    }

#if NX_DEBUG_CONSOLE
    cout << "NEW -------------------------------\n\n";
    for (const auto& editor : m_editors)
    {
        cout << (editor.getFileName().empty() ? "[new file]" : editor.getFileName()) << endl;
    }
    cout << endl;
#endif
}

void EditorWindow::openFile(const string& fileName /* = string() */)
{
    const char* fn = 0;
    if (fileName.empty())
    {
        const char* filters[] = { "*.asm", "*.s" };
        fn = tinyfd_openFileDialog("Load source code", 0, sizeof(filters) / sizeof(filters[0]),
            filters, "Source code", 0);
    }
    else
    {
        fn = fileName.c_str();
    }

    if (fn)
    {
        // Make sure we don't already have it open.
        for (int i = 0; i < getNumEditors(); ++i)
        {
            if (getEditor(i).getFileName() == fn)
            {
                switchTo(getEditor(i));
                return;
            }
        }

        newFile();

        Editor& thisEditor = getEditor();
        if (thisEditor.getData().load(fn))
        {
            thisEditor.setFileName(fn);
        }
        else
        {
            string msg = stringFormat("Unable to open file '{0}'.", fileName);
            tinyfd_messageBox("ERROR", msg.c_str(), "ok", "warning", 0);
            closeFile();
        }
    }

#if NX_DEBUG_CONSOLE
    cout << "NEW -------------------------------\n\n";
    for (const auto& editor : m_editors)
    {
        cout << (editor.getFileName().empty() ? "[new file]" : editor.getFileName()) << endl;
    }
    cout << endl;
#endif
}

void EditorWindow::switchTo(const Editor& editor)
{
    int editorIndex = int(&editor - m_editors.data());
    int tabIndex = int(find(m_editorOrder.begin(), m_editorOrder.end(), editorIndex) - m_editorOrder.begin());
    m_editorOrder.erase(m_editorOrder.begin() + tabIndex);
    m_editorOrder.insert(m_editorOrder.begin(), editorIndex);
}

void EditorWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    bool resetStatus = true;

    if (key == K::LShift || key == K::RShift || key == K::LAlt || key == K::RAlt ||
        key == K::LControl || key == K::RControl || key == K::Unknown)
    {
        resetStatus = false;
    }

    if (down && ctrl && !shift && !alt)
    {
        switch (key)
        {
        case K::N:  // New file
            newFile();
            break;

        case K::W:  // Close file
            closeFile();
            break;

        case K::O:  // Open file
            openFile();
            break;
        }
    }

    if (!ctrl && !alt && key == K::F4)
    {
        if (down)
        {
            if (shift)
            {
                if (--m_currentError < 0) m_currentError = max(0, (int)m_errors.size() - 1);
            }
            else
            {
                if (++m_currentError >= m_errors.size()) m_currentError = 0;
            }
            goToError(m_currentError);
        }
        resetStatus = false;
    }

    if (!m_editors.empty())
    {
        if (m_selectedTab == -1) getEditor().key(key, down, shift, ctrl, alt);

        if (down && ctrl && !alt && !shift)
        {
            if (key == K::Tab)
            {
                if (m_selectedTab == -1)
                {
                    // First time selecting tab
                    m_selectedTab = 1;
                }
                else
                {
                    ++m_selectedTab;
                }
                if (m_selectedTab >= m_editors.size()) m_selectedTab = 0;
            }
        }

        if ((m_selectedTab >= 0) && !down && !ctrl && !shift && !alt)
        {
            int index = m_editorOrder[m_selectedTab];
            // #todo: use switchTo
            m_editorOrder.erase(m_editorOrder.begin() + m_selectedTab);
            m_editorOrder.insert(m_editorOrder.begin(), index);
            m_selectedTab = -1;
        }

        // Set the title
        string title = getEditor().getTitle();
        setTitle(string("Editor/Assembler - ") + title);
    }
    else
    {
        setTitle(string("Editor/Assembler"));
    }

    if (resetStatus)
    {
        setDefaultStatus();
    }
}

void EditorWindow::onText(char ch)
{
    if (!m_editors.empty())
    {
        getEditor().text(ch);
    }
    setDefaultStatus();
}

bool EditorWindow::saveAll()
{
    bool asked = false;
    bool saveUnnamedFiles = false;

    if (m_editors.empty()) return true;

    for (auto& editor : m_editors)
    {
        if (!editor.getData().hasChanged()) continue;

        const string& fileName = editor.getFileName();

        if (!asked && fileName.empty())
        {
            int result = tinyfd_messageBox(
                "Unsaved files detected",
                "There are some new files open in the editor that are unsaved.  Do you still wish to save "
                "these files before continuing?",
                "yesnocancel", "question", 0);
            bool skipThisFile = false;
            switch (result)
            {
            case 0:     // Cancel - stop everything!
                return false;
            case 1:     // Yes - trigger save of unnamed/unsaved files
                asked = true;
                saveUnnamedFiles = true;
                break;
            case 2:     // No - do not save
                asked = true;
                saveUnnamedFiles = false;
                break;
            }
        }

        if (!fileName.empty() || saveUnnamedFiles)
        {
            editor.save(editor.getFileName());
        }
    }

    return true;
}

bool EditorWindow::needToSave() const
{
    for (auto& editor : m_editors)
    {
        if (editor.getData().hasChanged()) return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Error cycling
//----------------------------------------------------------------------------------------------------------------------

void EditorWindow::setErrorInfos(vector<Assembler::ErrorInfo> errors)
{
    m_errors = errors;
    m_currentError = -1;
}

void EditorWindow::goToError(int n)
{
    if (n < 0 || n >= m_errors.size()) return;
    const Assembler::ErrorInfo& err = m_errors[n];

    openFile(err.m_fileName);
    EditorData& data = getEditor().getData();

    // Make sure we have a valid line
    if ((err.m_line - 1) < data.getNumLines())
    {
        EditorData::Pos pos = data.getPosAtLine(err.m_line - 1);

        // Make sure we have a valid column
        if ((err.m_column - 1) < data.lineLength(err.m_line - 1))
        {
            pos += err.m_column - 1;
        }

        data.moveTo(pos);
    }

    setStatus(err.m_error, Draw::attr(Colour::Black, Colour::Red, true));
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

