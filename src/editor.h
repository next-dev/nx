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
    SplitView(const vector<char>& v, size_t start, size_t end);
    SplitView(const vector<char>& v, size_t start1, size_t end1, size_t start2, size_t end2);

    char operator[] (size_t n) const
    {
        return (n < m_end[0])
            ? m_array[m_start[0] + n]
            : (n < m_end[1])
                ? m_array[m_start[1] + n]
                : ' ';
    }
    
    size_t size() const
    {
        return m_end[0] - m_start[0] + m_end[1] - m_start[1];
    }

public:
    const vector<char>& m_array;
    size_t m_start[2];
    size_t m_end[2];
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
    EditorData(size_t initialSize, size_t increaseSize);

    void clear();
    SplitView getLine(int n) const;
    SplitView getText() const;
    size_t lineLength(int n) const;
    
    bool insert(char ch);
    bool backspace();
    
private:
    bool ensureSpace(size_t numChars);

private:
    vector<char>    m_buffer;
    vector<size_t>  m_lines;
    int             m_currentLine;
    size_t          m_cursor;
    size_t          m_endBuffer;
    size_t          m_increaseSize;
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
    bool key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt);
    bool text(char ch);

    void clear();
    
private:
    EditorData      m_data;
    int             m_x;
    int             m_y;
    int             m_width;
    int             m_height;
    int             m_topLine;
    int             m_currentLine;
    int             m_currentX;
    bool            m_font6;
    u8              m_bkgColour;
    vector<bool>    m_allowedChars;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

