//----------------------------------------------------------------------------------------------------------------------
// Tape browser
//----------------------------------------------------------------------------------------------------------------------

#include <emulator/nx.h>
#include <tape/tape.h>

#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// Tape
//----------------------------------------------------------------------------------------------------------------------

Tape::Tape()
    : m_currentBlock(-1)
    , m_state(State::Stopped)
    , m_index(0)
    , m_bitIndex(15)
    , m_counter(0)
{

}

Tape::Tape(const vector<u8>& data)
    : Tape()
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
        hdr.u.p.autoStartLine = WORD_OF(tapeHdr, 14);
        hdr.u.p.programLength = WORD_OF(tapeHdr, 16);
        hdr.u.p.variableOffset = WORD_OF(tapeHdr, 12) - hdr.u.p.programLength;
        break;

    case 1: // Number array
        hdr.u.a.variableName = BYTE_OF(tapeHdr, 15) - 129 + 'A';
        hdr.u.a.arrayLength = (WORD_OF(tapeHdr, 12) - 3) / 5;
        break;

    case 2: // String array
        hdr.u.a.variableName = BYTE_OF(tapeHdr, 15) - 193 + 'A';
        hdr.u.a.arrayLength = WORD_OF(tapeHdr, 12) - 3;
        break;

    case 3: // Bytes
        hdr.u.b.startAddress = WORD_OF(tapeHdr, 14);
        hdr.u.b.dataLength = WORD_OF(tapeHdr, 12);
        break;

    default:
        NX_ASSERT(0);
    }

    return hdr;
}

//----------------------------------------------------------------------------------------------------------------------
// Tape header control
//----------------------------------------------------------------------------------------------------------------------

void Tape::play()
{
    if (m_state == State::Stopped)
    {
        m_index = 0;
        m_bitIndex = 15;
        m_counter = 6988800;
        m_state = State::Quiet;
    }
}

void Tape::stop()
{
    m_state = State::Stopped;
    m_index = 0;
    m_bitIndex = 0;
    m_counter = 0;
}

void Tape::toggle()
{
    if (m_state == State::Stopped)
    {
        play();
    }
    else
    {
        stop();
    }
}

void Tape::selectBlock(int i)
{
    m_currentBlock = i;
}

u8 Tape::play(TState tStates)
{
    m_counter -= (int)tStates;
    u8 result = 0;

    static int bits = 0;

    for (;;)
    {
        switch (m_state)
        {
        case State::Stopped:
            m_counter = 0;
            result = 0;
            break;

        case State::Quiet:
            if (m_counter <= 0)
            {
                if (m_currentBlock == m_blocks.size())
                {
                    m_state = State::Stopped;
                    m_index = 0;
                    m_bitIndex = 0;
                    m_counter = 0;
                    m_currentBlock = 0;
                    return true;
                }

                // End of quiet, start header or data pilot
                if (m_blocks[m_currentBlock][0])
                {
                    // Data block
                    m_counter = 3222 * 2168;
                }
                else
                {
                    // Header block
                    m_counter = 8059 * 2168;
                }
                m_state = State::Pilot;
                continue;
            }
            result = 1;
            break;

        case State::Pilot:
            if (m_counter <= 0)
            {
                // Transition to sync1
                m_counter += 667;
                m_state = State::Sync1;
                continue;
            }
            result = (u8)((m_counter / 2168) & 1);
            break;

        case State::Sync1:
            if (m_counter <= 0)
            {
                // Transition to sync2
                m_counter += 735;
                m_state = State::Sync2;
                continue;
            }
            result = 1;
            break;

        case State::Sync2:
            if (m_counter <= 0)
            {
                // Transition to Data
                m_bitIndex = 15;
                bits = 0;
                nextBit();
                continue;
            }
            result = 0;
            break;

        case State::Data:
            if (m_counter <= 0)
            {
                nextBit();
            }

            result = !(m_bitIndex & 1);
            break;
        }

        break;
    }

    static TState total = 0;
    static u8 lastBit = result;

    total += tStates;
    if (result != lastBit)
    {
        // Edge detected
        NX_LOG("Edge after: %dT [%d->%d]\n", (int)total, lastBit, result);
        total = 0;
        if (bits++ == 16)
        {
            NX_LOG("--------------------------------------------------\n");
            bits = 0;
        }
    }
    lastBit = result;


    return result << 6;
}

bool Tape::nextBit()
{
    m_state = State::Data;

    // Check for end of block
    if (m_index == m_blocks[m_currentBlock].size())
    {
        // Next block
        m_state = State::Quiet;
        m_counter = 6988800;
        m_index = 0;
        m_bitIndex = 15;
        ++m_currentBlock;
        return true;
    }

    u8 b = m_blocks[m_currentBlock][m_index];

    // Notice the right shift.  We process each bit twice so can do a high and low pulse per bit
    u8 bit = b & (1 << (m_bitIndex >> 1));

    m_counter += bit ? 1710 : 855;

    if (--m_bitIndex < 0)
    {
        m_bitIndex += 16;
        ++m_index;
    }

    return false;
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
                    desc2 = draw.format("auto: %d, length: %d", hdr.u.p.autoStartLine, hdr.u.p.programLength);
                }
                break;

            case Tape::BlockType::NumberArray:
                {
                    category = "NUMBER ARRAY";
                    Tape::Header hdr = m_tape->getHeader(i);
                    desc1 = draw.format("\"%s\"", hdr.fileName.c_str());
                    desc2 = draw.format("name: %c, length: %d", hdr.u.a.variableName, hdr.u.a.arrayLength);
                }
                break;

            case Tape::BlockType::StringArray:
                {
                    category = "STRING ARRAY";
                    Tape::Header hdr = m_tape->getHeader(i);
                    desc1 = draw.format("\"%s\"", hdr.fileName.c_str());
                    desc2 = draw.format("name: %c$, length: %d", hdr.u.a.variableName, hdr.u.a.arrayLength);
                }
                break;

            case Tape::BlockType::Bytes:
                {
                    category = "       BYTES";
                    Tape::Header hdr = m_tape->getHeader(i);
                    desc1 = draw.format("\"%s\"", hdr.fileName.c_str());
                    desc2 = draw.format("start: $%04x, length: %d", hdr.u.b.startAddress, hdr.u.b.dataLength);
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

            draw.printString(m_x + 2, y, category.c_str(), false, colour);
            draw.printSquashedString(m_x + 16, y, desc1.c_str(), colour);
            draw.printSquashedString(m_x + 16, y + 1, desc2.c_str(), colour);

            if (m_tape->getCurrentBlock() == i)
            {
                draw.printChar(m_x + 1, y, m_tape->isPlaying() ? '*' : ')', colour, gGfxFont);
            }
        }
    }
}

void TapeWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;

    if (!m_tape) return;
    if (!down) return;

    int halfSize = (m_height - 2) / 4;

    if (!shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::Up:
            if (m_index > 0)
            {
                --m_index;
                while (m_index < m_topIndex)
                {
                    m_topIndex = max(0, m_topIndex - ((m_height - 2) / 4));
                }
            }
            break;

        case K::Down:
            if (m_index < (m_tape->numBlocks() - 1))
            {
                ++m_index;
                if ((m_index >= (m_topIndex + halfSize)) &&
                    (m_tape->numBlocks() > (2 * halfSize)))
                {
                    // Cursor has gone past halfway on a list that's bigger than the window
                    ++m_topIndex;
                }
            }
            break;

        case K::Return:
            m_tape->stop();
            m_tape->selectBlock(m_index);
            break;
                
        default:
            break;
        }
    }
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
        "Esc/Ctrl-T|Exit",
        "Up|Cursor up",
        "Down|Cursor down",
        "Enter|Select tape position",
        "Ctrl-Space|Play/Stop"
        })
    , m_currentTape(nullptr)
{

}

Tape* TapeBrowser::loadTape(const vector<u8>& data)
{
    if (m_currentTape)
    {
        delete m_currentTape;
    }

    m_currentTape = new Tape(data);
    m_window.setTape(m_currentTape);
    return m_currentTape;
}

void TapeBrowser::render(Draw& draw)
{
    m_window.draw(draw);
}

void TapeBrowser::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;

    if (down && !shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::Escape:
            getEmulator().hideAll();
            break;

        default:
            if (down) m_window.keyPress(key, down, shift, ctrl, alt);
        }
    }
    else if (down && !shift && ctrl && !alt)
    {
        switch (key)
        {
        case K::Space:
            if (m_currentTape) m_currentTape->toggle();
            break;

        case K::T:
            getEmulator().hideAll();
            break;
                
        default:
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
