//----------------------------------------------------------------------------------------------------------------------
//! @file       editor/data.cc
//! @brief      Implements the editor buffer.
//----------------------------------------------------------------------------------------------------------------------

#include <editor/data.h>
#include <emulator/nxfile.h>
#include <utils/tinyfiledialogs.h>

static const int kInitialGapSize = 4096;

//----------------------------------------------------------------------------------------------------------------------

EditorData::EditorData()
    : m_fileName()
    , m_buffer()
    , m_gapStart(0)
    , m_gapEnd(kInitialGapSize)
{
    m_lines.push_back(0);
    m_currentLine = 0;
    m_buffer.resize(kInitialGapSize);
}

//----------------------------------------------------------------------------------------------------------------------

EditorData::EditorData(string fileName)
    : m_fileName(fileName)
    , m_buffer()
    , m_gapStart(0)
    , m_gapEnd(0)
{
    m_lines.push_back(0);
    m_currentLine = 0;
    if (NxFile::loadTextFile(fileName, m_buffer))
    {
        // Move the text to the end of an enlarged buffer.
        ensureGapSize(kInitialGapSize);
    }
    else
    {
        tinyfd_messageBox("LOADING ERROR", stringFormat("Unable to load {0}!", fileName).c_str(), "ok", "error", 0);
        m_buffer.resize(kInitialGapSize);
        m_gapEnd = kInitialGapSize;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void EditorData::clear()
{
    m_gapStart = 0;
    m_gapEnd = (i64)m_buffer.size();
    m_fileName.clear();
}

//----------------------------------------------------------------------------------------------------------------------

Pos EditorData::bufferPosToPos(BufferPos p) const
{
    return p > m_gapStart ? (p - (m_gapEnd - m_gapStart)) : p;
}

//----------------------------------------------------------------------------------------------------------------------

BufferPos EditorData::posToBufferPos(Pos p) const
{
    return p > m_gapStart ? m_gapStart + (p - m_gapStart) : p;
}

//----------------------------------------------------------------------------------------------------------------------

void EditorData::setInsertPoint(Pos pos)
{
    BufferPos bp = posToBufferPos(pos);
    if (pos < m_gapStart)
    {
        // Shift all data between pos and gap to the end of the gap, moving the gap to the left at the same time.
        i64 delta = m_gapStart - bp;
        move(m_buffer.begin() + pos, m_buffer.begin() + m_gapStart, m_buffer.begin() + m_gapEnd - delta);
        m_gapStart -= delta;
        m_gapEnd -= delta;
    }
    else
    {
        // Shift all data between the gap end and position to the beginning of the gap, moving the gap right.
        i64 delta = bp - m_gapEnd;
        move_backward(m_buffer.begin() + m_gapEnd, m_buffer.begin() + bp, m_buffer.begin() + m_gapStart);
        m_gapStart += delta;
        m_gapEnd += delta;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void EditorData::ensureGapSize(i64 size)
{
    if ((m_gapEnd - m_gapStart) < size)
    {
        // We need to resize the buffer to allow the extra size.
        i64 oldSize = (i64)m_buffer.size();
        i64 newSize = oldSize * 3 / 2;
        i64 minSize = oldSize + (size - (m_gapEnd - m_gapStart));
        i64 finalSize = std::max(newSize, minSize);
        i64 delta = oldSize - m_gapEnd;
        m_buffer.resize(finalSize);
        move_backward(m_buffer.begin() + m_gapEnd, m_buffer.begin() + m_gapEnd + delta,
            m_buffer.end() - delta);
        m_gapEnd += (finalSize - oldSize);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void EditorData::insert(Pos p, const char *start, const char* end)
{
    ensureGapSize(end - start);
    setInsertPoint(p);
    while (start < end) m_buffer[m_gapStart++] = *start;
}

//----------------------------------------------------------------------------------------------------------------------

string EditorData::makeString() const
{
    size_t s1 = (size_t)m_gapStart;
    size_t s2 = m_buffer.size() - m_gapEnd;

    string s;
    s.reserve(s1 + s2);
    s.append(m_buffer.data(), s1);
    s.append(m_buffer.data() + m_gapEnd, s2);
    return s;
}

//----------------------------------------------------------------------------------------------------------------------

EditorData::Line EditorData::getLine(Pos p) const
{
    Line l;
    BufferPos bp = posToBufferPos(p);
    if (bp == (BufferPos)m_buffer.size()) return { nullptr, nullptr, nullptr, nullptr, p };

    l.s1 = m_buffer.data() + bp;

    for (; bp < m_gapStart && m_buffer[bp] != '\n'; ++bp);
    l.e1 = m_buffer.data() + p;

    if (bp == m_gapStart)
    {
        l.s2 = m_buffer.data() + bp;
        for (bp = m_gapEnd; bp < (BufferPos)m_buffer.size() && m_buffer[bp] != '\n'; ++bp);
        l.e2 = m_buffer.data() + bp;
    }
    else
    {
        l.s2 = l.e2 = nullptr;
    }

    l.newPos = bufferPosToPos(bp) + 1;

    return l;
}

//----------------------------------------------------------------------------------------------------------------------

