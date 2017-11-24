//----------------------------------------------------------------------------------------------------------------------
// Editor UI
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "config.h"
#include "ui.h"

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

class EditorData
{
public:
    EditorData(int initialSize, int increaseSize, int maxLineLength);

    void clear();
    SplitView getLine(int n) const;
    SplitView getText() const;
    int lineLength(int n) const;
    int dataLength() const;
    
    void insert(char ch);
    void backspace();
    void deleteChar(int num);
    void moveTo(int pos);
    void leftChar(int num);
    void rightChar(int num);
    void upChar(int num);
    void downChar(int num);
    void newline();

    //
    // Queries
    //
    int getCurrentLine() const { return m_currentLine; }
    int getCurrentPosInLine() const;
    
private:
    bool ensureSpace(int numChars);
    void dump() const;
    int toVirtualPos(int actualPos) const;

private:
    vector<char>    m_buffer;
    vector<int>     m_lines;
    int             m_currentLine;
    int             m_cursor;
    int             m_endBuffer;
    int             m_increaseSize;
    int             m_maxLineLength;
    int             m_lastOffset;       // Used for remembering the line offset when moving up and down
};

//----------------------------------------------------------------------------------------------------------------------
// Editor
// If height == 1, then no newline characters will be inserted.
//----------------------------------------------------------------------------------------------------------------------

class Editor
{
public:
    Editor(int xCell, int yCell, int width, int height, u8 bkgColour, bool font6,
        int initialSize, int increaseSize);
    
    void onlyAllowDecimal();
    void onlyAllowHex();

    SplitView getText() const;

    void render(Draw& draw, int line);
    void renderAll(Draw& draw);
    bool key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt);
    bool text(char ch);

    void clear();

    EditorData& getData() { return m_data; }
    const EditorData& getData() const { return m_data; }
    
private:
    EditorData      m_data;
    int             m_x;
    int             m_y;
    int             m_width;
    int             m_height;
    int             m_topLine;
    bool            m_font6;
    u8              m_bkgColour;
    vector<bool>    m_allowedChars;
};

//----------------------------------------------------------------------------------------------------------------------
// EditorWindow
//----------------------------------------------------------------------------------------------------------------------

class EditorWindow final : public Window
{
public:
    EditorWindow(Nx& nx, string title);

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

private:
    Editor      m_editor;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

