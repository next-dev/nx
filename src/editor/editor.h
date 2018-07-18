//----------------------------------------------------------------------------------------------------------------------
// Editor UI
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <asm/asm.h>
#include <config.h>
//#include <utils/ui.h>

#include <functional>
#include <math.h>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
// SplitView
//----------------------------------------------------------------------------------------------------------------------

class SplitView
{
public:
    SplitView(const vector<char>& v, int start, int end);
    SplitView(const vector<char>& v, int start1, int end1, int start2, int end2);

    char operator[] (int n) const
    {
        //  +-----+-------+------+
        //  |XXXXX|       |XXXXXX|
        //  +-----+-------+------+
        //  S0    E0      S1     E1

        int l0 = m_end[0] - m_start[0];
        int l1 = m_end[1] - m_start[1];

        if ((n + m_start[0]) < m_end[0])
        {
            // Index is first part
            return m_array[m_start[0] + n];
        }
        else if (n - l0 < l1)
        {
            // Index is in second part
            return m_array[m_start[1] + (n - l0)];
        }
        else return ' ';
    }
    
    int size() const
    {
        return m_end[0] - m_start[0] + m_end[1] - m_start[1];
    }

public:
    const vector<char>& m_array;
    int m_start[2];
    int m_end[2];
};

//----------------------------------------------------------------------------------------------------------------------
// EditorData
//
// Data takes the form of a buffer:
//
//      +----------------------+----------------------+--------------------+
//      | Text before cursor   |                      | Text after cursor  |
//      +----------------------+----------------------+--------------------+
//                             ^                      ^
//                             |                      |
//                             cursor                 end-buffer
//
//----------------------------------------------------------------------------------------------------------------------

template <int n>
struct PosT
{
    int m_pos;

    PosT() : m_pos(-1) {}
    PosT(int p) : m_pos(p) {}
    PosT(const PosT& p) : m_pos(p.m_pos) {}

    PosT& operator= (int p) { m_pos = p; return *this; }
    PosT& operator= (const PosT& p) { m_pos = p.m_pos; return *this; }

    PosT& operator+= (int p) { m_pos += p; return *this; }
    PosT& operator-= (int p) { m_pos += p; return *this; }

    explicit operator int() const { return m_pos; }

    int operator- (const PosT& p) const { return (m_pos - p.m_pos); }
    PosT operator+ (int p) const { return (m_pos + p); }
    PosT operator- (int p) const { return (m_pos - p); }

    PosT& operator++ () { ++m_pos; return *this; }
    PosT& operator-- () { --m_pos; return *this; }
    PosT operator++ (int) { PosT p(m_pos); ++m_pos; return p; }
    PosT operator-- (int) { PosT p(m_pos); --m_pos; return p; }

    bool operator< (const PosT& p) const { return m_pos < p.m_pos; }
    bool operator> (const PosT& p) const { return m_pos > p.m_pos; }
    bool operator<= (const PosT& p) const { return m_pos <= p.m_pos; }
    bool operator>= (const PosT& p) const { return m_pos >= p.m_pos; }
    bool operator== (const PosT& p) const { return m_pos == p.m_pos; }
    bool operator!= (const PosT& p) const { return m_pos != p.m_pos; }

    bool isValid() const { return m_pos != -1; }

};

class EditorData
{
public:
    EditorData(int initialSize, int increaseSize);

    // This type represents a 0-based index from the beginning of the document, irregardless of the internal
    // editor buffer structure.  All public functions deal with this type.
    using Pos = PosT<0>;

    void clear();

    //
    // Query
    //
    SplitView getLine(int n) const;
    SplitView getText() const;
    vector<u8> getData() const;
    string getString() const;
    int lineLength(int n) const;
    int dataLength() const;
    int getNumLines() const;
    Pos lastWordPos() const;
    Pos nextWordPos() const;
    int getCurrentLine() const { return m_currentLine; }
    int getCurrentPosInLine() const;
    Pos getPosAtLine(int l) const;
    bool hasChanged() const { return m_changed; }
    char getChar(Pos p) const { return m_buffer[(int)toDataPos(p)]; }
    void setChar(Pos p, char ch) { m_buffer[(int)toDataPos(p)] = ch; }
    string getSearchTerm() const { return m_searchString; }

    Pos startPos() const { return toVirtualPos(DataPos(0)); }
    Pos endPos() const { return toVirtualPos(DataPos((int)m_buffer.size())); }
    Pos cursorPos() const { return toVirtualPos(m_cursor); }

    //
    // Commands
    //
    void insert(char ch);
    void insert(string str);
    void backspace(int num);
    void deleteChar(int num);
    void moveTo(Pos pos);
    void leftChar(int num);
    void rightChar(int num);
    void upChar(int num);
    void downChar(int num);
    void newline(bool indent = true);
    void home();
    void end();
    void cutLine();
    void copyLine();
    void pasteLine();
    bool findString(string str);
    void setReplaceTerm(string str);
    bool findNext();
    bool findPrev();
    bool replace();

    //
    // File
    //
    bool load(const char* fileName);
    bool save(const char* fileName);

    //
    // State
    //
    void resetChanged() { m_changed = false; }

    //
    // Tabs
    //
    void tab();
    void untab();
    void setTabs(vector<int> tabs, int tabSize);
    int lastTabPos() const;

private:
    // This type represents a true index into the internal buffer.  This only represents an actual
    // index into the document if the position is before the current cursor position, due to the
    // gap buffer structure that is used internally.  All positions that use
    using DataPos = PosT<1>;

    DataPos startDataPos() const { return DataPos(0); }
    DataPos cursorDataPos() const { return m_cursor; }
    DataPos afterCursorDataPos() const { return m_endBuffer; }
    DataPos endDataPos() const { return DataPos((int)m_buffer.size()); }

    bool ensureSpace(int numChars);
    void dump() const;
    Pos toVirtualPos(DataPos actualPos) const;
    DataPos toDataPos(Pos virtualPos) const;
    void changed() { m_changed = true; }
    static int getCharCategory(char ch);        // 0 = space, 1 = chars/numbers, 2 = punctuation

    bool isValidPos(Pos pos) const;
    bool isValidPos(DataPos pos) const;
    DataPos getLinePos(int line) const { return m_lines[line]; }
    void setLinePos(int line, DataPos pos) { NX_ASSERT(isValidPos(pos)); m_lines[line] = pos; }
    char getChar(DataPos p) const { NX_ASSERT(isValidPos(p));  return m_buffer[(int)p]; }
    void setChar(DataPos p, char ch) { NX_ASSERT(isValidPos(p));  m_buffer[(int)p] = ch; }

    DataPos find(const string& str, DataPos start, DataPos end, bool forwardSearch);

private:
    vector<char>    m_buffer;
    vector<DataPos> m_lines;
    int             m_currentLine;
    DataPos         m_cursor;
    DataPos         m_endBuffer;
    int             m_increaseSize;
    int             m_lastOffset;       // Used for remembering the line offset when moving up and down
    bool            m_changed;          // True, if the data changed since last reset
    vector<int>     m_initialTabs;
    int             m_tabSize;
    vector<char>    m_clipboard;
    string          m_searchString;
    string          m_replaceString;
};

//----------------------------------------------------------------------------------------------------------------------
// Editor
// If height == 1, then no newline characters will be inserted.
//----------------------------------------------------------------------------------------------------------------------

class Draw;

class Editor
{
public:
    using EnterFunction = function<void(Editor&)>;
    Editor(int xCell, int yCell, int width, int height, u8 bkgColour, bool font6,
        int initialSize, int increaseSize, EnterFunction onEnter = {});
    
    void onlyAllowDecimal();
    void onlyAllowHex();
    void setCommentColour(u8 colour);
    void setLineNumberColour(u8 colour);

    SplitView getText() const;
    string getTitle() const;

    void render(Draw& draw, int line, int lineNumberGap = 0);
    void renderAll(Draw& draw);
    bool key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt);
    bool text(char ch);

    void clear();
    void ensureVisibleCursor();

    EditorData& getData() { return m_data; }
    const EditorData& getData() const { return m_data; }
    string getFileName() const { return m_fileName; }
    void setFileName(string fileName) { m_fileName = fileName; }
    void save(string fileName = string());

    void setPosition(int xCell, int yCell, int width, int height);
    int getLineNumberGap(Draw& draw) const;
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    u8 getBkgColour() const { return m_bkgColour; }

private:
    EditorData          m_data;
    int                 m_x;
    int                 m_y;
    int                 m_width;
    int                 m_height;
    int                 m_topLine;
    int                 m_lineOffset;
    bool                m_font6;
    u8                  m_bkgColour;
    u8                  m_commentColour;
    u8                  m_lineNumberColour;
    vector<bool>        m_allowedChars;
    string              m_fileName;
    bool                m_ioAllowed;
    EnterFunction       m_onEnter;
    bool                m_showLineNumbers;
    int                 m_lineNumberWidthCache;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
