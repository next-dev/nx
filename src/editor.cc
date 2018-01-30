//----------------------------------------------------------------------------------------------------------------------
// Editor implementation
//----------------------------------------------------------------------------------------------------------------------

#include "editor.h"

#include <fstream>
#include "tinyfiledialogs.h"

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

EditorData::EditorData(int initialSize, int increaseSize, int maxLineLength)
    : m_buffer(16)
    , m_lines(1, 0)
    , m_cursor(0)
    , m_currentLine(0)
    , m_endBuffer((int)m_buffer.size())
    , m_increaseSize(increaseSize)
    , m_maxLineLength(maxLineLength)
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

int EditorData::getNumLines() const
{
    return int(m_lines.size());
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
    changed();
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
    changed();
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
    changed();
}

void EditorData::newline()
{
    insert(0x0a);
    m_lines.insert(m_lines.begin() + m_currentLine + 1, m_cursor);
    ++m_currentLine;
    DUMP();
    changed();
}

void EditorData::home()
{
    moveTo(toVirtualPos(m_lines[m_currentLine]));
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
        moveTo(toVirtualPos(m_lines[m_currentLine+1])-1);
    }
    m_lastOffset = -1;
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

void EditorData::untab()
{
    // Calculate last tab position
    int x = getCurrentPosInLine();
    int tab = 0;

    if (!m_initialTabs.empty() && x > m_initialTabs.back())
    {
        // We're in normal tabs territory.
        tab = m_initialTabs.back() +
            (((x - m_initialTabs.back()) - 1) / m_tabSize);
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

    // tab points to the place we should go.  Keep deleting spaces until we reach a non-space or the tab position
    while (x > tab)
    {
        if (m_buffer[m_cursor - 1] != ' ') break;
        backspace();
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
    return m_cursor - m_lines[m_currentLine];
}

int EditorData::getPosAtLine(int line) const
{
    return m_lines[line];
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
        int len = (int)(m_buffer.end() - (m_buffer.begin() + m_endBuffer));
        move(m_buffer.begin() + m_endBuffer, m_buffer.begin() + oldSize, m_buffer.end() - len);
        m_endBuffer += delta;
    }
    
    return result;
    
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
        for (int i = 0; i < m_cursor; ++i)
        {
            if (m_buffer[i] == 0x0a)
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
        f.write(m_buffer.data(), (size_t)m_cursor);
        f.write(m_buffer.data() + m_endBuffer, m_buffer.size() - m_endBuffer);
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

Editor::Editor(int xCell, int yCell, int width, int height, u8 bkgColour, bool font6, int initialSize, int increaseSize)
    : m_data(initialSize, increaseSize, width-1)
    , m_x(xCell)
    , m_y(yCell)
    , m_width(width)
    , m_height(height)
    , m_topLine(0)
    , m_font6(font6)
    , m_bkgColour(bkgColour)
    , m_commentColour(bkgColour)
    , m_allowedChars(128, true)
    , m_ioAllowed(increaseSize != 0)
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

void Editor::render(Draw& draw, int line)
{
    EditorData& data = getData();
    u8 colour = m_bkgColour;
    int y = line - m_topLine + 1;
    if (y < m_height)
    {
        // This line is visible
        int x = m_x;
        y += m_y;

        auto view = data.getLine((int)line);
        for (int i = 0; x < m_x + m_width; ++x, ++i)
        {
            if (view[i] == ';') colour = m_commentColour;
            draw.printChar(x, y, view[i], colour);
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
    int y = m_y + 1;
    int endY = y + m_height;

    for (; y < endY; ++y)
    {
        render(draw, line++);
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
        m_topLine = std::min(m_data.getNumLines(), m_topLine + K_LINE_SKIP);

        // Check to see if visible yet.  If not, just jump.
        if (m_data.getCurrentLine() >= (m_topLine + m_height))
        {
            m_topLine = std::max(0, m_data.getCurrentLine() - (m_height / 2));
        }
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
            break;

        case K::Right:
            m_data.rightChar(1);
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
            break;

        case K::Home:
            m_data.home();
            break;

        case K::End:
            m_data.end();
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

        case K::O:  // Open file...
            if (m_ioAllowed)
            {
                if (m_data.hasChanged())
                {
                    // Check to see if the user really wants to overwrite their changes.
                    if (!tinyfd_messageBox("Are you sure?", "There has been changes since you last saved.  Are you sure you want to lose your changes.",
                        "yesno", "question", 0))
                    {
                        break;
                    }
                }

                const char* filters[] = { "*.asm", "*.s" };
                const char* fileName = tinyfd_openFileDialog("Load source code", 0, sizeof(filters)/sizeof(filters[0]),
                    filters, "Source code", 0);
                if (fileName)
                {
                    if (m_data.load(fileName))
                    {
                        setFileName(fileName);
                    }
                    else
                    {
                        tinyfd_messageBox("ERROR", "Unable to open file!", "ok", "warning", 0);
                    }
                }
            }
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
    : Window(nx, 1, 1, 78, 60, title, Colour::Blue, Colour::Black, false)
    , m_editors()
    , m_index(0)
{
    newFile();
    getEditor().setCommentColour(Draw::attr(Colour::Green, Colour::Black, false));
}

void EditorWindow::onDraw(Draw& draw)
{
    getEditor().renderAll(draw);

    // Draw tab bar
    draw.attrRect(m_x, m_y+1, m_width, 1, Draw::attr(Colour::Blue, Colour::White, false));

    // Draw tabs
    int x = m_x + 1;
    int y = m_y + 1;
    for (int i = 0; i < m_editors.size(); ++i)
    {
        string tabName = m_editors[i].getTitle();
        if (tabName.length() > 16)
        {
            // Shorten it
            tabName = tabName.substr(tabName.size() - 13, 13) + "...";
        }
        tabName = string(" ") + tabName + " ";

        int len = draw.squashedStringWidth(tabName);
        if (x + len >= (m_x + m_width - 1))
        {
            break;
        }

        x += draw.printSquashedString(x, y, tabName, Draw::attr(Colour::White, Colour::Red, i == m_index)) + 1;
    }
}

void EditorWindow::newFile()
{
    m_index = int(m_editors.size());
    m_editors.emplace_back(2, 2, 76, 58, Draw::attr(Colour::White, Colour::Black, false), false, 1024, 1024);
    m_editors.back().setCommentColour(Draw::attr(Colour::Green, Colour::Black, false));
}

void EditorWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    if (down && ctrl && !shift && !alt)
    {
        switch (key)
        {
        case K::N:  // New file
            newFile();
            break;
        }
    }
    getEditor().key(key, down, shift, ctrl, alt);
}

void EditorWindow::onText(char ch)
{
    getEditor().text(ch);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

