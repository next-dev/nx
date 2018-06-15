//----------------------------------------------------------------------------------------------------------------------
// Disassembler document
//----------------------------------------------------------------------------------------------------------------------

#include <asm/disasm.h>
#include <disasm/disassembler.h>
#include <emulator/nxfile.h>
#include <utils/tinyfiledialogs.h>
#include <utils/ui.h>

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

DisassemblerDoc::DisassemblerDoc(Spectrum& speccy)
    : m_speccy(&speccy)
{
    switch (speccy.getModel())
    {
    case Model::ZX48:
        m_mmap.resize(65536);
        for (int i = 0; i < 65536; ++i)
        {
            m_mmap[i] = speccy.peek(u16(i));
        }
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
    case Model::ZXNext:
        break;
            
    default:
        NX_ASSERT(0);
    }

    reset();
}

//----------------------------------------------------------------------------------------------------------------------
// Internal functions
//----------------------------------------------------------------------------------------------------------------------

void DisassemblerDoc::reset()
{
    m_lines.clear();

    switch (m_speccy->getModel())
    {
    case Model::ZX48:
        {
            MemAddr start = m_speccy->convertAddress(Z80MemAddr(0x4000));
            MemAddr end = m_speccy->convertAddress(Z80MemAddr(0xffff));
            m_lines.emplace_back(LineType::UnknownRange, -1, start, end, string{});
        }
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
    case Model::ZXNext:
        break;

    default:
        assert(0);
    }

    m_changed = false;
}

bool DisassemblerDoc::processCommand(CommandType type, int line, MemAddr addr, string text)
{
    int commandIndex = (int)m_commands.size();

    switch (type)
    {
    case CommandType::FullComment:
        NX_ASSERT(line >= 0 && line <= int(m_lines.size()));
        m_lines.emplace(m_lines.begin() + line, LineType::FullComment, commandIndex, MemAddr(), MemAddr(), text);
        break;

    case CommandType::LineComment:
        NX_ASSERT(line >= 0 && line <= int(m_lines.size()));
        NX_ASSERT(m_lines[line].type == LineType::Instruction);
        m_lines[line].text = text;
        break;

    case CommandType::CodeEntry:
        // Make sure we don't already have this entry point.
        {
            if (optional<int> lineIndex = findLine(addr); lineIndex)
            {
                int i = *lineIndex;
                Line& line = getLine(i);

                NX_ASSERT(line.type != LineType::Blank);
                NX_ASSERT(line.type != LineType::FullComment);
                if (line.type == LineType::Label ||
                    line.type == LineType::Instruction)
                {
                    Overlay::currentOverlay()->error("Code already generated for this entry point");
                    return false;
                }
                else
                {
                    // Get memory addresses:
                    //
                    //  +------+-----------+------------+
                    //  a1     addr        c           a3
                    //
                    MemAddr a1 = line.startAddress;
                    MemAddr a3 = line.endAddress;
                    MemAddr c = addr;

                    i = deleteLine(i);
                    if (c - a1 >= 0)
                    {
                        insertLine(i++, Line{ LineType::UnknownRange, commandIndex, a1, addr - 1, {} });
                        insertLine(i++, Line{ LineType::Blank, commandIndex, {}, {}, {} });
                    }

                    insertLine(i++, Line{ LineType::Label, commandIndex, c, c, text });

                    Disassembler dis;
                    bool endFound = false;
                    while (!endFound && (c < a3))
                    {
                        // Grab the next 4 bytes
                        // #todo: refactor the memory system out so we can clone it and not use memory maps
                        u16 a = m_speccy->convertAddress(c);
                        u8 b1 = m_mmap[a];
                        u8 b2 = (a <= 0xfffe) ? m_mmap[a + 1] : 0;
                        u8 b3 = (a <= 0xfffd) ? m_mmap[a + 2] : 0;
                        u8 b4 = (a <= 0xfffc) ? m_mmap[a + 3] : 0;
                        u16 na = dis.disassemble(a, b1, b2, b3, b4);
                        c = m_speccy->convertAddress(Z80MemAddr(a));
                        MemAddr nc = m_speccy->convertAddress(Z80MemAddr(na));

                        // Lets look at the opcode to see if we continue.  We stop at JP, RET, RETI and RETN
                        switch (b1)
                        {
                        case 0xc3:      // JP nnnn
                        case 0xc9:      // RET
                        case 0xe9:      // JP (HL)
                            endFound = true;
                            break;

                        case 0xed:
                            switch (b2)
                            {
                            case 0x45:
                            case 0x4d:
                            case 0x55:
                            case 0x5d:
                            case 0x65:
                            case 0x6d:
                            case 0x75:
                            case 0x7d:
                                endFound = true;

                            default:
                                break;
                            }
                            break;

                        case 0xdd:      // IX
                        case 0xfd:      // IY
                            endFound = (b2 == 0xe9);    // JP (IX+n)/(IY+n)
                            break;
                        }

                        // Add a line for the code
                        insertLine(i++, Line{ commandIndex, c, nc - 1, dis.opCodeString(), dis.operandString() });

                        c = nc;
                        a = na;
                    }

                    if (c != a3)
                    {
                        insertLine(i++, Line{ LineType::Blank, commandIndex, {}, {}, {} });
                        insertLine(i++, Line{ LineType::UnknownRange, commandIndex, c, a3, {} });
                    }
                }
            }
            else
            {
                Overlay::currentOverlay()->error("Invalid code entry point.");
                return false;
            }
        }
        break;

    default:
        NX_ASSERT(0);
    }

    m_commands.emplace_back(type, line, addr, text);
    changed();
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

int DisassemblerDoc::deleteLine(int line)
{
    int commandIndex = m_lines[line].commandIndex;

    bool confirmDelete = false;
    if (m_lines[line].type == LineType::Instruction ||
        m_lines[line].type == LineType::Label)
    {
        for (const auto& line : m_lines)
        {
            if (line.commandIndex == commandIndex && !m_commands[line.commandIndex].text.empty())
            {
                confirmDelete = true;
                break;
            }
        }
    }

    bool shouldDelete = confirmDelete
        ? tinyfd_messageBox("Are you sure?", "Comments or labels have been added to the area you will delete.  Are you sure you want to lose your changes?",
            "yesno", "question", 0)
        : true;

    if (shouldDelete)
    {
        if (m_lines[line].type == LineType::UnknownRange)
        {
            // Unknown ranges are always generated and not from a command.  They are also only a single line.
            m_lines.erase(m_lines.begin() + line);
        }
        else
        {
            // Adjust the result index according to how many lines are deleted before the current line
            int l = line;
            for (int i = 0; i < l; ++i)
            {
                if (m_lines[i].commandIndex == commandIndex) --line;
            }

            // Delete the command
            m_commands.erase(m_commands.begin() + commandIndex);

            // Replay all the commands
            replayCommands();
        }
    }

    return line;
}

void DisassemblerDoc::replayCommands()
{
    m_lines.clear();

    MemAddr start = m_speccy->convertAddress(Z80MemAddr(0x4000));
    MemAddr end = m_speccy->convertAddress(Z80MemAddr(0xffff));
    m_lines.emplace_back(LineType::UnknownRange, -1, start, end, string{});

    for (const auto& command : m_commands)
    {
        processCommand(command.type, command.line, command.addr, command.text);
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
        if (f.checkSection('MM48', 0))
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
                MemAddr addr = dcmd.peekAddr(x + 5);
                string text = dcmd.peekString(x + 9);
                x += (9 + (int)text.size() + 1);

                if (!processCommand(type, line, addr, text)) return false;
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
    BlockSection mm48('MM48', 0);
    NX_ASSERT(m_mmap.size() == 65536);
    mm48.pokeData(m_mmap);
    f.addSection(mm48);

    //
    // DCMD section
    //
    BlockSection dcmd('DCMD', 0);
    dcmd.poke32(u32(m_commands.size()));
    for (const auto& command : m_commands)
    {
        dcmd.poke8((u8)command.type);
        dcmd.poke32(command.line);
        dcmd.pokeAddr(command.addr);
        dcmd.pokeString(command.text);
    }
    f.addSection(dcmd);
    m_changed = false;

    return f.save(fileName);
}

optional<int> DisassemblerDoc::findLine(MemAddr addr) const
{
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        const Line& line = m_lines[i];
        if (line.type == LineType::UnknownRange || line.type == LineType::Instruction)
        {
            if (addr >= line.startAddress && addr <= line.endAddress)
            {
                return int(i);
            }
        }
        else if (line.type == LineType::Label)
        {
            if (addr == line.startAddress) return int(i);
        }
    }

    return {};
}
