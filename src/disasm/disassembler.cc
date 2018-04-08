//----------------------------------------------------------------------------------------------------------------------
// Disassembler document
//----------------------------------------------------------------------------------------------------------------------

#include <disasm/disassembler.h>
#include <emulator/nxfile.h>
#include <utils/tinyfiledialogs.h>

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

DisassemblerDoc::DisassemblerDoc(Spectrum& speccy)
{
    switch (speccy.getModel())
    {
    case Model::ZX48:
        m_mmap.resize(65536);
        for (int i = 0; i < 65536; ++i)
        {
            m_mmap[0] = speccy.peek(u16(i));
        }
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
        assert(0);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Internal functions
//----------------------------------------------------------------------------------------------------------------------

void DisassemblerDoc::reset()
{
    m_lines.emplace_back(LineType::UnknownRange, -1, 0x4000, 0xffff, string{});
    m_changed = false;
}

bool DisassemblerDoc::processCommand(CommandType type, int line, i64 param1, string text)
{
    int commandIndex = (int)m_commands.size();
    m_commands.emplace_back(type, line, param1, text);
    changed();

    switch (type)
    {
    case CommandType::FullComment:
        assert(line >= 0 && line <= int(m_lines.size()));
        m_lines.emplace(m_lines.begin() + line, LineType::FullComment, commandIndex, 0, 0, text);
        break;

    case CommandType::LineComment:
        assert(line >= 0 && line <= int(m_lines.size()));
        assert(m_lines[line].type == LineType::Instruction);
        m_lines[line].text = text;
        break;

    case CommandType::CodeEntry:
        break;

    default:
        assert(0);
    }

    return true;
}

void DisassemblerDoc::setText(int commandIndex, string text)
{
    m_commands[commandIndex].text = text;
    find_if(m_lines.begin(), m_lines.end(), [commandIndex](const Line& line) { return line.commandIndex == commandIndex; })->text = text;
}

int DisassemblerDoc::getCommandIndex(int line) const
{
    return m_lines[line].commandIndex;
}

void DisassemblerDoc::deleteLine(int line)
{
    int commandIndex = m_lines[line].commandIndex;

    bool confirmDelete = false;
    for (const auto& line : m_lines)
    {
        if (line.commandIndex == commandIndex &&
            !line.text.empty())
        {
            confirmDelete = true;
            break;
        }
    }

    bool shouldDelete = confirmDelete
        ? tinyfd_messageBox("Are you sure?", "Comments have been added to the area you will delete.  Are you sure you want to lose your changes?",
            "yesno", "question", 0)
        : true;

    if (shouldDelete)
    {
        // Remove all lines that reference the deleted command
        vector<Line> newLines;
        copy_if(m_lines.begin(), m_lines.end(), back_inserter(newLines), [commandIndex](const Line& line) {
            return line.commandIndex != commandIndex;
        });
        m_lines = newLines;

        // Delete the command
        m_commands.erase(m_commands.begin() + commandIndex);

        // Adjust command index references in lines to cater for missing command
        for (Line& line : m_lines)
        {
            if (line.commandIndex > commandIndex) --line.commandIndex;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// File operations
//
// Disassembly file format:
//
// Uses NX file format.
//
// BLOCK TYPES & FORMATS:
//
//      MM48 (length = 65536)
//          Offset  Length  Description
//          0       65536   Bytes
//
//      DCMD
//          Offset  Length  Description
//          0       4       Number of commands
//          4       ?       Commands of the format:
//
//              Offset  Length  Description
//              0       1       Type
//              1       4       Line
//              5       8       Parameter 1
//              13      ?       Text
//----------------------------------------------------------------------------------------------------------------------

bool DisassemblerDoc::load(string fileName)
{
    reset();

    NxFile f;
    if (f.load(fileName))
    {
        if (f.checkSection('MM48', 65536))
        {
            const BlockSection& mm48 = f['MM48'];
            mm48.peekData(0, m_mmap, 65536);
        }
        else
        {
            return false;
        }

        if (f.checkSection('DCMD', 0))
        {
            const BlockSection& dcmd = f['DCMD'];
            int numCmds = (int)dcmd.peek32(0);
            int x = 4;
            for (int i = 0; i < numCmds; ++i)
            {
                CommandType type = (CommandType)dcmd.peek8(x + 0);
                int line = (int)dcmd.peek32(x + 1);
                i64 p1 = dcmd.peek64(x + 5);
                string text = dcmd.peekString(x + 13);
                x += (13 + (int)text.size() + 1);

                if (!processCommand(type, line, p1, text)) return false;
            }
        }
    }

    m_changed = false;
    return true;
}

bool DisassemblerDoc::save(string fileName)
{
    NxFile f;

    //
    // MM48 section
    //
    BlockSection mm48('MM48');
    assert(m_mmap.size() == 65536);
    mm48.pokeData(m_mmap);
    f.addSection(mm48, 65536);

    //
    // DCMD section
    //
    BlockSection dcmd('DCMD');
    dcmd.poke32(u32(m_commands.size()));
    for (const auto& command : m_commands)
    {
        dcmd.poke8((u8)command.type);
        dcmd.poke32(command.line);
        dcmd.poke64(command.param1);
        dcmd.pokeString(command.text);
    }
    f.addSection(dcmd, 0);
    m_changed = false;

    return f.save(fileName);
}

