//----------------------------------------------------------------------------------------------------------------------
//! @file       editor/data.h
//! @brief      Defines the EditorData class that manages a buffer of text data.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <string>
#include <tuple>
#include <vector>

using BufferPos = i64;      //!< Actual position in the buffer including the gap.
using Pos = i64;            //!< Virtual position in the buffer, ignoring the gap.

class EditorData
{
public:
    EditorData();
    EditorData(string fileName);

    Pos bufferPosToPos(BufferPos p) const;
    BufferPos posToBufferPos(Pos p) const;

    void bufferLineText(i64 line, const char** s0, const char** e0, const char** s1, const char** e1);

    string makeString() const;

    //
    // Queries
    //
    char getChar(Pos p) { return m_buffer[posToBufferPos(p)]; }

    struct Line
    {
        const char*     s1;     // Start of first part of line
        const char*     e1;     // End of first part of line
        const char*     s2;     // Start of second part of line (or nullptr if no second part)
        const char*     e2;     // End of second part of line (or nullptr if no second part)
        Pos             newPos; // New position of next line
    };
    Line getLine(Pos pos) const;
    Pos lastPos() const;

    i64 getLineNumber(Pos p) const;
    Pos getLinePos(i64 line) const;


    //
    // Commands
    //
    void clear();
    void insert(Pos p, const char *start, const char* end);
    void insert(Pos p, char ch) { insert(p, &ch, &ch + 1); }
    void insert(Pos p, string str) { insert(p, str.data(), str.data() + str.size()); }

private:
    void setInsertPoint(Pos pos);
    void ensureGapSize(i64 size);

private:
    string              m_fileName;     //!< The name of the file where this text came from.
    vector<char>        m_buffer;       //!< The text data
    vector<BufferPos>   m_lines;
    BufferPos           m_gapStart;     //!< Where the gap starts.
    BufferPos           m_gapEnd;       //!< Where the gap ends.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
