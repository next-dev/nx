//----------------------------------------------------------------------------------------------------------------------
//! @file       editor/data.h
//! @brief      Defines the EditorData class that manages a buffer of text data.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <string>
#include <vector>

using BufferPos = i64;      //!< Actual position in the buffer including the gap.
using Pos = i64;            //!< Virtual position in the buffer, ignoring the gap.

class EditorData
{
public:
    EditorData();
    EditorData(string fileName);

    Pos bufferPosToPos(BufferPos p);
    BufferPos posToBufferPos(Pos p);

    void bufferLineText(i64 line, const char** s0, const char** e0, const char** s1, const char** e1);

    string makeString() const;

    //
    // Commands
    //
    void clear();
    void insert(Pos p, const char *start, const char* end);
    void insert(Pos p, char ch) { insert(p, &ch, &ch + 1); }
    void insert(Pos p, string str) { insert(p, &*str.begin(), &*str.end()); }

private:
    void setInsertPoint(Pos pos);
    void ensureGapSize(i64 size);

private:
    string              m_fileName;     //!< The name of the file where this text came from.
    vector<char>        m_buffer;       //!< The text data
    vector<BufferPos>   m_lines;
    int                 m_currentLine;
    BufferPos           m_gapStart;     //!< Where the gap starts.
    BufferPos           m_gapEnd;       //!< Where the gap ends.
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
