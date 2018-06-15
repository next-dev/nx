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

    bool _load(string fileName);
    bool _save(string fileName);
    bool hasChanged() const { return m_changed; }

    int getNumLines() const { return (int)m_lines.size(); }
    int deleteLine(int line);

    //
    // Use cases
    //

    void insertComment(int line, string comment);
    void setComment(int line, string comment);
    int generateCode(MemAddr addr, string label);

    //
    // Lines
    // Used for rendering and generated from commands via processCommand() function.
    //
    enum class LineType
    {
        Blank,              // Blank Line
        UnknownRange,       // Unknown range of memory
        FullComment,        // Line-based comment
        Label,
        Instruction,
    };

    struct Line
    {
        LineType    type;
        int         commandIndex;   // Index of command that generated this line
        MemAddr     startAddress;
        MemAddr     endAddress;
        string      text;
        string      opCode;
        string      operand;

        Line(LineType type, int commandIndex, MemAddr start, MemAddr end, string text, string opCode, string operand)
            : type(type)
            , commandIndex(commandIndex)
            , startAddress(start)
            , endAddress(end)
            , text(move(text))
            , opCode(move(opCode))
            , operand(move(operand))
        {}

        Line(LineType type, int commandIndex, MemAddr start, MemAddr end, string text)
            : Line(type, commandIndex, start, end, text, {}, {})
        {}

        Line(int commandIndex, MemAddr start, MemAddr end, string opCode, string operands)
            : Line(LineType::Instruction, commandIndex, start, end, {}, opCode, operands)
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

private:
    const Spectrum*     m_speccy;
    vector<u8>          m_mmap;         // Snapshot of memory
    bool                m_changed;

    //
    // Internal database generated from processing commands
    //
    vector<Line>        m_lines;
};
