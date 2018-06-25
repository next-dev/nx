//----------------------------------------------------------------------------------------------------------------------
// Disassembler document
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <config.h>
#include <types.h>
#include <asm/asm.h>
#include <emulator/spectrum.h>

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

    string addLabel(string label, MemAddr addr);
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
    int generateData(MemAddr addr, DataType type, int size);
    void increaseDataSize(int line, int maxSize);
    void decreaseDataSize(int line);
    void addNewDataLine(int line);

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
        DataBytes,
        DataString,
        DataWords,
        END
    };

    struct Line
    {
        int             tag;
        LineType        type;
        Disassembler    disasm;
        MemAddr         startAddress;
        string          text;
        int             size;

        Line(int tag, LineType type, MemAddr start, string text, int size)
            : tag(tag)
            , type(type)
            , startAddress(start)
            , text(move(text))
            , size(size)
        {}
    };

    const Line& getLine(int i) const { return m_lines[i]; }
    Line& getLine(int i) { return m_lines[i]; }

    void insertLine(int i, Line line) { m_lines.insert(m_lines.begin() + i, line); }

private:
    //
    // Internal API
    //

    void reset();
    void changed() { m_changed = true; }
    void checkBlankLines(int line);
    bool middleOfCode(int line) const;

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
};
