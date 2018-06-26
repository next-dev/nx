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
    , m_nextTag(1)
{
    insertLine(0, Line{ 0, LineType::END, {}, {}, {}, 0 });
    switch (speccy.getModel())
    {
    case Model::ZX48:
        {
            m_mmap.resize(65536);
            m_mtype.resize(65536, false);
            for (int i = 0; i < 65536; ++i)
            {
                m_mmap[i] = speccy.peek(u16(i));
            }
            MemAddr start = m_speccy->convertAddress(Z80MemAddr(0x4000));
            MemAddr end = m_speccy->convertAddress(Z80MemAddr(0xffff));
        }
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
    case Model::ZXNext:
        break;
            
    default:
        NX_ASSERT(0);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Internal functions
//----------------------------------------------------------------------------------------------------------------------

void DisassemblerDoc::reset()
{
    m_lines.clear();
    m_labelMap.clear();
    m_addrMap.clear();
    m_changed = false;
    for (int i = 0; i < 65536; ++i) m_mtype[i] = false;
}

void DisassemblerDoc::insertBlankLine(int line, int tag)
{
    insertLine(line, Line{ tag, LineType::Blank, {}, {}, {}, 0 });
}

bool DisassemblerDoc::middleOfCode(int line) const
{
    if (line == 0) return false;

    const Line& line1 = getLine(line - 1);
    const Line& line2 = getLine(line);

    bool code1 = line1.type == LineType::Label || line1.type == LineType::Instruction;
    bool code2 = line2.type == LineType::Label || line2.type == LineType::Instruction;

    return (code1 && code2);
}

int DisassemblerDoc::insertComment(int line, int tag, string comment)
{
    if (middleOfCode(line))
    {
        // We're insert a blank first then the comment.  We also make sure the comment shares the same tag
        // as the surrounding code.
        Line& l = getLine(line);
        tag = l.tag;
        insertLine(line, Line{ tag, LineType::FullComment, l.startAddress, {}, comment, 0 });
        insertLine(line, Line{ tag, LineType::Blank, l.startAddress, {}, {}, 0 });
        return line + 1;
    }
    else
    {
        // The blank line comes after the comment in this case.  However, if there is already a blank line
        // we re-tag it to belong to this comment.  If the following line is another comment, we do not
        // insert a blank line.
        Line& l = getLine(line);
        if (l.type == LineType::Blank)
        {
            l.tag = tag;
        }
        else if (l.type != LineType::FullComment)
        {
            insertLine(line, Line{ tag, LineType::Blank, l.startAddress, {}, {}, 0 });
        }

        Line& l2 = getLine(line);
        insertLine(line, Line{ tag, LineType::FullComment, l2.startAddress, {}, comment, 0 });
        return line;
    }
    changed();
}

void DisassemblerDoc::setComment(int line, string comment)
{
    NX_ASSERT(m_lines[line].type == LineType::FullComment || m_lines[line].type == LineType::Instruction);
    if (m_lines[line].text != comment)
    {
        m_lines[line].text = comment;
        changed();
    }
}

MemAddr DisassemblerDoc::disassemble(Disassembler& dis, MemAddr addr) const
{
    u16 a = m_speccy->convertAddress(addr);
    u8 b1 = m_mmap[a];
    u8 b2 = (a <= 0xfffe) ? m_mmap[a + 1] : 0;
    u8 b3 = (a <= 0xfffd) ? m_mmap[a + 2] : 0;
    u8 b4 = (a <= 0xfffc) ? m_mmap[a + 3] : 0;
    u16 na = dis.disassemble(a, b1, b2, b3, b4);
    return m_speccy->convertAddress(Z80MemAddr(na));
}

int DisassemblerDoc::generateCode(MemAddr addr, int tag, string label)
{
    if (optional<int> lineIndex = findLine(addr); lineIndex)
    {
        int i = *lineIndex;
        MemAddr end;

        if (i < getNumLines())
        {
            // Code at this address goes to the end of the file
            Line& line = getLine(i);

            NX_ASSERT(line.type != LineType::Blank);

            // Find the maximum end point (which is that start address of the section after it).
            end = line.startAddress;
            if (end <= addr)
            {
                Overlay::currentOverlay()->error("Code/Data already generated for this entry point");
                return -1;
            }
        }
        else
        {
            end = m_speccy->convertAddress(Z80MemAddr(0xffff));
        }

        MemAddr c = addr;

        int startLine = i;

        // Insert a blank line if the previous line is not blank
        if (startLine > 0 && getLine(startLine - 1).type != LineType::Blank)
        {
            insertBlankLine(i++, tag);
        }

        // Insert the label
        insertLine(i++, Line{ tag, LineType::Label, c, label, {}, 0 });

        Disassembler dis;
        bool endFound = false;
        while (!endFound && (c < end))
        {
            // Grab the next 4 bytes
            // #todo: refactor the memory system out so we can clone it and not use memory maps
            u16 c16 = m_speccy->convertAddress(c);
            if (m_mtype[c16])
            {
                Overlay::currentOverlay()->error("Code already generated for this entry point");
                return -1;
            }
            Line l{ tag, LineType::Instruction, c, {}, {}, 0 };
            MemAddr nc = disassemble(l.disasm, c);
            l.size = (nc - c);

            // Lets look at the opcode to see if we continue.  We stop at JP, RET, RETI and RETN
            switch (m_mmap[c16])
            {
            case 0xc3:      // JP nnnn
            case 0xc9:      // RET
            case 0xe9:      // JP (HL)
                endFound = true;
                break;

            case 0xed:
                switch (m_mmap[(c16 + 1)])
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
                endFound = (m_mmap[c16 + 1] == 0xe9);    // JP (IX+n)/(IY+n)
                break;
            }

            // Add a line for the code
            insertLine(i++, l);
            int sizeInstruction = nc - c;
            for (; sizeInstruction > 0; --sizeInstruction) m_mtype[c16++] = true;

            c = nc;
        }

        if (c != end)
        {
            insertBlankLine(i, tag);
        }

        changed();
        return startLine;
    }
    else
    {
        Overlay::currentOverlay()->error("Invalid code entry point.");
        return -1;
    }
}

int DisassemblerDoc::generateData(MemAddr addr, int tag, DataType type, int size, string label)
{
    // #todo: refactor this to use shared code with generateCode
    if (optional<int> lineIndex = findLine(addr); lineIndex)
    {
        int i = *lineIndex;
        MemAddr end;

        if (i < getNumLines())
        {
            Line& line = getLine(i);
            NX_ASSERT(line.type != LineType::Blank);

            // Find the maximum end-point
            end = line.startAddress;
            if (end <= addr)
            {
                Overlay::currentOverlay()->error("Code/Data already generated for this entry point");
                return -1;
            }
        }
        else
        {
            end = m_speccy->convertAddress(Z80MemAddr(0xffff));
        }

        // Check to see if we have room
        u16 a = m_speccy->convertAddress(addr);
        int bytes = size;
        if (type == DataType::Word) bytes *= 2;
        if (int(a) + bytes > 65535)
        {
            Overlay::currentOverlay()->error("No room for data");
            return -1;
        }
        for (int i = 0; i < bytes; ++i)
        {
            if (m_mtype[a + i])
            {
                Overlay::currentOverlay()->error("Code/Data already generated for this entry point");
                return -1;
            }
        }

        // Generate the lines
        // Insert a blank line if the previous line is not blank
        int startLine = i;
        if (startLine > 0 && getLine(startLine - 1).type != LineType::Blank)
        {
            insertBlankLine(i++, tag);
            ++startLine;
        }

        // Figure out the line type from the data type
        LineType lt;
        switch (type)
        {
        case DataType::Byte:    lt = LineType::DataBytes;       break;
        case DataType::String:  lt = LineType::DataString;      break;
        case DataType::Word:    lt = LineType::DataWords;       break;

        default:
            NX_ASSERT(0);
        }

        // Insert a label if the label is greater than 6 characters
        string dataLabel;
        if (label.size() > 6)
        {
            Line l{ tag, LineType::Label, addr, label, {}, 0 };
            insertLine(i++, l);
        }
        else
        {
            dataLabel = label;
        }

        // Insert the data line
        Line l{ tag, lt, addr, dataLabel, {}, bytes };
        insertLine(i++, l);

        if ((addr + bytes) != end)
        {
            insertBlankLine(i, tag);
        }

        changed();
        return startLine;
    }
    else
    {
        Overlay::currentOverlay()->error("Invalid code entry point.");
        return -1;
    }
}

bool DisassemblerDoc::replaceLabel(int line, string oldLabel, string newLabel)
{
    NX_ASSERT(getLine(line).type == LineType::Label);

    auto itLabel = m_labelMap.find(oldLabel);
    NX_ASSERT(itLabel != m_labelMap.end());

    MemAddr a = itLabel->second.second;

    m_labelMap.erase(itLabel);
    m_addrMap.erase(m_addrMap.find(a));

    auto it = m_labelMap.find(newLabel);
    if (it == m_labelMap.end())
    {
        // New label doesn't exist so let's add it.
        addLabel(newLabel, a);
        getLine(line).label = newLabel;
        changed();
        return true;
    }

    return false;
}

int DisassemblerDoc::deleteLine(int line)
{
    int tag = m_lines[line].tag;
    int newLine = line;
    bool unknownRangeFound = false;
    MemAddr start, end;

    // Check to see if we're deleting a full-line comment.
    if (m_lines[line].type == LineType::FullComment)
    {
        // We just delete the comment line
        m_lines.erase(m_lines.begin() + line);
        if ((line > 0) &&
            (m_lines[line-1].type == LineType::Blank) &&
            (m_lines[line].type != LineType::FullComment))
        {
            // Erase the blank too
            m_lines.erase(m_lines.begin() + line - 1);
        }
        return line;
    }

    //
    // Otherwise we delete the section and clean up blank lines
    //

    // Ensure the return value (the final line selected) is correct.
    //
    for (int i = 0; i < line; ++i)
    {
        Line& line = m_lines[i];
        if (line.tag == tag) --newLine;
    }

    // Remove any labels from the database.
    //
    for_each(m_lines.begin(), m_lines.end(), [this, tag](Line& line) {
        if (line.tag == tag && line.type == LineType::Label)
        {
            auto it1 = m_labelMap.find(line.label);
            auto it2 = m_addrMap.find(line.startAddress);
            m_labelMap.erase(it1);
            m_addrMap.erase(it2);
        }
    });

    // Remove the imprint on the m_mtype array.
    //
    for_each(m_lines.begin(), m_lines.end(), [this, tag](Line& line) {
        if (line.tag == tag &&
            (line.type == LineType::Instruction ||
                line.type == LineType::DataBytes ||
                line.type == LineType::DataString ||
                line.type == LineType::DataWords))
        {
            u16 a = m_speccy->convertAddress(line.startAddress);
            for (int i = 0; i < line.size; ++i)
            {
                m_mtype[a++] = false;
            }
        }
    });

    // Remove the lines
    //
    auto startRemovals = remove_if(m_lines.begin(), m_lines.end(), [tag](const auto& line) {
        return line.tag == tag;
    });

    m_lines.erase(startRemovals, m_lines.end());

    while ((newLine < (int)m_lines.size()) &&
        (m_lines[newLine].type == LineType::Blank))
    {
        m_lines.erase(m_lines.begin() + newLine);
    }

    checkBlankLines(newLine);

    changed();
    return newLine;
}

void DisassemblerDoc::checkBlankLines(int line)
{
    if (line == 0) return;

    Line& line1 = getLine(line - 1);
    Line& line2 = getLine(line);

    // If either line is a blank then nothing to do.
    if (line1.type == LineType::Blank || line2.type == LineType::Blank) return;

    // Detect a border (where tags differ, except with comments).
    if (line1.tag != line2.tag &&
        !(line1.type == LineType::FullComment && line2.type == LineType::FullComment))
    {
        // There's a border
        insertBlankLine(line, line1.tag);
    }
}

optional<u16> DisassemblerDoc::extractAddress(int lineNum) const
{
    Disassembler dis;
    const Line& line = getLine(lineNum);
    if (line.type != LineType::Instruction) return {};
    MemAddr m = disassemble(dis, line.startAddress);
    return dis.extractAddress();
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
            m_mtype.resize(65536);
            for (int i = 0; i < 65536; ++i) m_mtype[i] = mm48.peek8(65536 + i);
        }
        else
        {
            return false;
        }

        if (f.checkSection('DCMD', 0))
        {
            const BlockSection& dcmd = f['DCMD'];
            int numLines = (int)dcmd.peek32(0);
            int x = 4;
            for (int i = 0; i < numLines; ++i)
            {
                int tag = dcmd.peek32(x);
                LineType type = (LineType)dcmd.peek8(x+4);
                MemAddr start = dcmd.peekAddr(x + 5);
                string label = dcmd.peekString(x + 9);
                x += (9 + (int)label.size() + 1);
                string text = dcmd.peekString(x);
                x += (int)text.size() + 1;
                u16 srcAddr = dcmd.peek16(x);
                x += 2;
                u8 b1 = dcmd.peek8(x++);
                u8 b2 = dcmd.peek8(x++);
                u8 b3 = dcmd.peek8(x++);
                u8 b4 = dcmd.peek8(x++);
                int size = (int)dcmd.peek8(x++);

                m_lines.emplace_back(tag, type, start, label, text, size);
                u16 na = m_lines.back().disasm.disassemble(srcAddr, b1, b2, b3, b4);
                for (int i = 0; i < (na - srcAddr); ++i)
                {
                    m_mtype[srcAddr++] = true;
                }

                if (!label.empty())
                {
                    NX_ASSERT(type != LineType::END && type != LineType::Blank && type != LineType::FullComment);
                    addLabel(label, start);
                }
            }
            m_nextTag = dcmd.peek32(x);
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
    for (const bool b : m_mtype) mm48.poke8(b ? 1 : 0);
    f.addSection(mm48);

    //
    // DCMD section
    //
    BlockSection dcmd('DCMD', 0);
    dcmd.poke32(u32(m_lines.size()));
    for (const auto& line : m_lines)
    {
        dcmd.poke32((u32)line.tag);
        dcmd.poke8((u8)line.type);
        dcmd.pokeAddr(line.startAddress);
        dcmd.pokeString(line.label);
        dcmd.pokeString(line.text);
        int i = 0;
        dcmd.poke16(line.disasm.srcZ80Addr());
        const vector<u8>& bytes = line.disasm.bytes();
        for (i = 0; i < (int)bytes.size(); ++i)
        {
            dcmd.poke8(bytes[i]);
        }
        for (; i < 4; ++i) dcmd.poke8(0);
        dcmd.poke8((u8)line.size);
    }
    dcmd.poke32((u32)m_nextTag);
    f.addSection(dcmd);

    m_changed = false;
    return f.save(fileName);
}

optional<int> DisassemblerDoc::findLine(MemAddr addr) const
{
    for (size_t i = 0; i < m_lines.size() - 1; ++i)
    {
        const Line& line = m_lines[i];
        if (addr <= line.startAddress)
        {
            return int(i);
        }
    }

    return (int)m_lines.size() - 1;
}

optional<int> DisassemblerDoc::findLabelLine(MemAddr addr) const
{
    for (size_t i = 0; i < m_lines.size() - 1; ++i)
    {
        const Line& line = m_lines[i];
        if (line.type != LineType::Label) continue;

        if (addr == line.startAddress)
        {
            return int(i);
        }

        if (addr < line.startAddress) return {};
    }

    return {};
}

string DisassemblerDoc::addLabel(string label, MemAddr addr)
{
    auto it = m_addrMap.find(addr);
    if (it != m_addrMap.end())
    {
        // We already have a label for this address.
        return it->second.first;
    }

    // This is a new label
    Disassembler::LabelInfo info = make_pair(label, addr);
    m_labelMap[label] = info;
    m_addrMap[addr] = info;
    return label;
}

