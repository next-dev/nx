//----------------------------------------------------------------------------------------------------------------------
// Tape browser
//----------------------------------------------------------------------------------------------------------------------

#include <algorithm>

#include "tape.h"
#include "nx.h"

//----------------------------------------------------------------------------------------------------------------------
// Tape
//----------------------------------------------------------------------------------------------------------------------

Tape::Tape()
    : m_currentBlock(-1)
{

}

Tape::Tape(const vector<u8>& data)
{
    const u8* t = data.data();
    const u8* end = data.data() + data.size();

    while (t < end)
    {
        u16 size = WORD_OF(t, 0);
        t += 2;
        m_blocks.emplace_back(t, t + size);
        t += size;
    }
}

Tape::BlockType Tape::getBlockType(int i) const
{
    BlockType result = BlockType::Block;

    u8 type = m_blocks[i][0];
    if (type == 0x00)
    {
        switch (m_blocks[i][1])
        {
        case 0x00:  result = BlockType::Program;        break;
        case 0x01:  result = BlockType::NumberArray;    break;
        case 0x02:  result = BlockType::StringArray;    break;
        case 0x03:  result = BlockType::Bytes;          break;
        default:
            result = BlockType::Block;
        }
    }

    return result;
}

u16 Tape::getBlockLength(int i) const
{
    return (u16)m_blocks[i].size();
}

Tape::Header Tape::getHeader(int i) const
{
    Header hdr;
    const u8* tapeHdr = m_blocks[i].data();
    for (int i = 0; i < 10; ++i)
    {
        hdr.fileName += tapeHdr[2 + i];
    }
    while (*(hdr.fileName.end() - 1) == ' ')
    {
        hdr.fileName.erase(hdr.fileName.end() - 1);
    }
    hdr.checkSum = tapeHdr[17];
    hdr.type = getBlockType(i);

    switch (tapeHdr[1])
    {
    case 0: // Program
        hdr.p.autoStartLine = WORD_OF(tapeHdr, 14);
        hdr.p.programLength = WORD_OF(tapeHdr, 16);
        hdr.p.variableOffset = WORD_OF(tapeHdr, 12) - hdr.p.programLength;
        break;

    case 1: // Number array
        hdr.a.variableName = BYTE_OF(tapeHdr, 15) - 129 + 'A';
        hdr.a.arrayLength = (WORD_OF(tapeHdr, 12) - 3) / 5;
        break;

    case 2: // String array
        hdr.a.variableName = BYTE_OF(tapeHdr, 15) - 193 + 'A';
        hdr.a.arrayLength = WORD_OF(tapeHdr, 12) - 3;
        break;

    case 3: // Bytes
        hdr.b.startAddress = WORD_OF(tapeHdr, 14);
        hdr.b.dataLength = WORD_OF(tapeHdr, 12);
        break;

    default:
        NX_ASSERT(0);
    }

    return hdr;
}

void Tape::selectBlock(int i)
{
    m_currentBlock = i;
}

//----------------------------------------------------------------------------------------------------------------------
// Tape window
//----------------------------------------------------------------------------------------------------------------------

TapeWindow::TapeWindow(Nx& nx)
    : Window(nx, 1, 1, 40, 60, "Tape Browser", Colour::Black, Colour::White, true)
    , m_topIndex(0)
    , m_index(0)
    , m_tape(nullptr)
{

}

void TapeWindow::reset()
{
    m_index = m_topIndex = 0;
}

void TapeWindow::onDraw(Draw& draw)
{
    if (!m_tape)
    {
        draw.printSquashedString(m_x + 2, m_y + 2, "No tape inserted.  Open a tape file. ",
            draw.attr(Colour::White, Colour::Red, true));
    }
    else
    {
        int numBlocks = m_tape->numBlocks();
        int y = m_y + 1;

        for (int i = m_topIndex; (i < numBlocks) && (y < (m_topIndex + m_height - 2)); i++, y+=2)
        {
            u8 colour = 0;

            if (i == m_index)
            {
                colour = draw.attr(Colour::Black, Colour::Yellow, true);
            }
            else
            {
                colour = draw.attr(Colour::Black, Colour::White, (y & 2) != 0);
            }
            draw.attrRect(m_x, y, m_width, 2, colour);

            Tape::BlockType type = m_tape->getBlockType(i);
            string category, desc1, desc2;
            switch (type)
            {
            case Tape::BlockType::Program:
                {
                    category = "     PROGRAM";
                    Tape::Header hdr = m_tape->getHeader(i);
                    desc1 = draw.format("\"%s\"", hdr.fileName.c_str());
                    desc2 = draw.format("auto: %d, length: %d", hdr.p.autoStartLine, hdr.p.programLength);
                }
                break;

            case Tape::BlockType::NumberArray:
                {
                    category = "NUMBER ARRAY";
                    Tape::Header hdr = m_tape->getHeader(i);
                    desc1 = draw.format("\"%s\"", hdr.fileName.c_str());
                    desc2 = draw.format("name: %c, length: %d", hdr.a.variableName, hdr.a.arrayLength);
                }
                break;

            case Tape::BlockType::StringArray:
                {
                    category = "STRING ARRAY";
                    Tape::Header hdr = m_tape->getHeader(i);
                    desc1 = draw.format("\"%s\"", hdr.fileName.c_str());
                    desc2 = draw.format("name: %c$, length: %d", hdr.a.variableName, hdr.a.arrayLength);
                }
                break;

            case Tape::BlockType::Bytes:
                {
                    category = "       BYTES";
                    Tape::Header hdr = m_tape->getHeader(i);
                    desc1 = draw.format("\"%s\"", hdr.fileName.c_str());
                    desc2 = draw.format("start: $%04x, length: %d", hdr.b.startAddress, hdr.b.dataLength);
                }
                break;

            case Tape::BlockType::Block:
                {
                    category = "       BLOCK";
                    desc1 = draw.format("Length: %d", m_tape->getBlockLength(i) - 2);
                    desc2 = "";
                }
                break;
            }

            draw.printString(m_x + 2, y, category.c_str(), colour);
            draw.printSquashedString(m_x + 16, y, desc1.c_str(), colour);
            draw.printSquashedString(m_x + 16, y + 1, desc2.c_str(), colour);

            if (m_tape->getCurrentBlock() == i)
            {
                draw.printChar(m_x + 1, y, ')', colour, gGfxFont);
            }
        }
    }
}

void TapeWindow::onKey(sf::Keyboard::Key key, bool shift, bool ctrl, bool alt)
{

}

void TapeWindow::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------
// Tape browser overlay
//----------------------------------------------------------------------------------------------------------------------

TapeBrowser::TapeBrowser(Nx& nx)
    : Overlay(nx)
    , m_window(nx)
    , m_commands({
        "Esc|Exit",
        })
    , m_currentTape(nullptr)
{

}

void TapeBrowser::loadTape(const vector<u8>& data)
{
    if (m_currentTape)
    {
        delete m_currentTape;
    }

    m_currentTape = new Tape(data);
    m_window.setTape(m_currentTape);
}

void TapeBrowser::render(Draw& draw)
{
    m_window.draw(draw);
}

void TapeBrowser::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    if (down && !shift && !ctrl && !alt)
    {
        using K = sf::Keyboard::Key;
        switch (key)
        {
        case K::Escape:
            getEmulator().hideAll();
            break;
        }
    }
}

void TapeBrowser::text(char ch)
{

}

const vector<string>& TapeBrowser::commands() const
{
    return m_commands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
