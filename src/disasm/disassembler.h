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
//----------------------------------------------------------------------------------------------------------------------

class DisassemblerDoc
{
public:
    DisassemblerDoc(Spectrum& speccy);

    bool load(string fileName);
    bool save(string fileName);
    bool hasChanged() const { return m_changed; }

    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt);
    void onText(char ch);

    int getNumLines() const { return (int)m_lines.size(); }

    enum class LineType
    {
        UnknownRange,       // Unknown range of memory
        FullComment,        // Line-based comment
        MajorLabel,
        MinorLabel,
        Instruction
    };

    struct Line
    {
        LineType                type;
        MemoryMap::Address      startAddress;
        MemoryMap::Address      endAddress;
        string                  text;

        Line(LineType type, MemoryMap::Address start, MemoryMap::Address end, string text)
            : type(type)
            , startAddress(start)
            , endAddress(end)
            , text(move(text))
        {}
    };

    const Line& getLine(int i) const { return m_lines[i]; }

private:
    //
    // Internal data structures
    //
    enum class CommandType
    {
        FullComment,        // Insert a full comment
        LineComment,        // Comment next to a line
        CodeEntry,          // Insert a code entry point
    };

    struct Command
    {
        CommandType     type;
        int             line;
        i64             param1;
        string          text;

        Command(CommandType type, int line, i64 param1, string text)
            : type(type)
            , line(line)
            , param1(param1)
            , text(move(text))
        {}
    };

private:
    //
    // Internal API
    //

    void reset();
    void changed() { m_changed = false; }
    bool processCommand(CommandType type, int line, i64 param1, string text = {});

private:
    vector<u8>          m_mmap;         // Snapshot of memory
    vector<Command>     m_commands;
    bool                m_changed;

    //
    // Internal database generated from processing commands
    //
    vector<Line>        m_lines;
};
