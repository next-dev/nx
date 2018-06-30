//----------------------------------------------------------------------------------------------------------------------
// Disassembler document
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <config.h>
#include <types.h>
#include <asm/asm.h>
#include <emulator/spectrum.h>
#include <set>

//----------------------------------------------------------------------------------------------------------------------
// DisassemblerDoc
//
// Command format:
//
//  Type            Line                Param1                              Text
//
//  FullComment     Insert point        Command index inserting before      Comment text
//  LineComment     Instruction pos     Command index generated code        Comment text
//  CodeEntry       Range line          -                                   -
//----------------------------------------------------------------------------------------------------------------------

class Spectrum;

class DisassemblerDoc
{
public:
    DisassemblerDoc(Spectrum& speccy);

    bool load(string fileName);
    bool save(string fileName);
    bool hasChanged() const { return m_changed; }

    int getNumLines() const { return (int)m_lines.size() - 1; }
    int deleteLine(int line);
    int getNextTag() { return m_nextTag++; }
    optional<int> findLine(MemAddr addr) const;
    optional<int> findLabelLine(MemAddr addr) const;
    optional<MemAddr> nextAddr(int line);
    u8 getByte(u16 addr) { return m_mmap[addr]; }
    u16 getWord(u16 addr) { return getByte(addr) + 256 * getByte(addr + 1); }

    string addLabel(string label, MemAddr addr);
    void removeLabel(string label);
    const map<MemAddr, Disassembler::LabelInfo>& getLabelsByAddr() const { return m_addrMap; }

    //
    // Use cases
    //

    void insertBlankLine(int line, int tag);
    int insertComment(int line, int tag, string comment);
    void setComment(int line, string comment);
    int generateCode(MemAddr addr, int tag, string label);
    bool replaceLabel(int line, string oldLabel, string newLabel);
    optional<u16> extractAddress(int line) const;
    MemAddr disassemble(Disassembler& dis, MemAddr addr) const;     // Return address points after instruction

    enum class DataType
    {
        Byte,
        String,
        Word,
        Binary,
    };
    int generateData(MemAddr addr, int tag, DataType type, int size, string label);
    int increaseDataSize(int line);
    int decreaseDataSize(int line);
    int setDataSize(int line, int size);

    //
    // Lines
    // Used for rendering and generated from commands via processCommand() function.
    //
    enum class LineType
    {
        Blank,              // Blank Line
        FullComment,        // Line-based comment
        Label,
        Instruction,
        DataBytes,          // db $01,$02,$03...
        DataString,         // db "text...",13,...
        DataWords,          // dw $0001,$0002,...
        END
    };

    struct Line
    {
        int             tag;
        LineType        type;
        Disassembler    disasm;
        MemAddr         startAddress;
        string          label;
        string          text;
        int             size;

        Line(int tag, LineType type, MemAddr start, string label, string text, int size)
            : tag(tag)
            , type(type)
            , startAddress(start)
            , label(label)
            , text(text)
            , size(size)
        {}
    };

    const Line& getLine(int i) const { return m_lines[i]; }
    Line& getLine(int i) { return m_lines[i]; }

    void insertLine(int i, Line line);

    //
    // Bookmarks
    //

    void toggleBookmark(int line);
    int nextBookmark(int currentLine);
    int prevBookmark(int currentLine);
    void checkBookmarksWhenRemovingLine(int line);
    void checkBookmarksWhenInsertingLine(int line);
    vector<int> enumBookmarks();

private:
    //
    // Internal API
    //

    void reset();
    void changed() { m_changed = true; }
    void checkBlankLines(int line);
    bool middleOfCode(int line) const;
    int numDataBytes(LineType type, int size) const;
    bool isData(int line) const;
    void deleteSingleLine(int line);

private:
    const Spectrum*     m_speccy;
    vector<u8>          m_mmap;         // Snapshot of memory
    vector<bool>        m_mtype;
    bool                m_changed;

    //
    // Internal database
    //
    vector<Line>        m_lines;
    int                 m_nextTag;

    map<string, Disassembler::LabelInfo>    m_labelMap;
    map<MemAddr, Disassembler::LabelInfo>   m_addrMap;

    vector<int>            m_bookmarks;
    vector<int>::iterator  m_currentBookmark;
};
