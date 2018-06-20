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
class Disassembler;

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

    string addLabel(string label, MemAddr addr);

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
        END
    };

    struct Line
    {
        int         tag;
        LineType    type;
        MemAddr     startAddress;
        MemAddr     endAddress;
        string      text;
        string      opCode;
        string      operand;

        Line(int tag, LineType type, MemAddr start, MemAddr end, string text, string opCode, string operand)
            : tag(tag)
            , type(type)
            , startAddress(start)
            , endAddress(end)
            , text(move(text))
            , opCode(move(opCode))
            , operand(move(operand))
        {}

        Line(int tag, LineType type, MemAddr start, MemAddr end, string text)
            : Line(tag, type, start, end, text, {}, {})
        {}

        Line(int tag, MemAddr start, MemAddr end, string opCode, string operands)
            : Line(tag, LineType::Instruction, start, end, {}, opCode, operands)
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
    optional<int> findLine(MemAddr addr) const;
    void checkBlankLines(int line);
    bool middleOfCode(int line) const;

private:
    const Spectrum*     m_speccy;
    vector<u8>          m_mmap;         // Snapshot of memory
    bool                m_changed;

    //
    // Internal database
    //
    vector<Line>        m_lines;
    int                 m_nextTag;

    using LabelInfo = pair<string, MemAddr>;
    map<string, LabelInfo>      m_labelMap;
    map<MemAddr, LabelInfo>     m_addrMap;
};
