//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/overlay_asm.h>
#include <utils/format.h>

#define NX_DEBUG_LOG_LEX    (0)

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Assembler::Assembler(AssemblerWindow& window, std::string initialFile, array<int, 4> pages)
    : m_assemblerWindow(window)
    , m_numErrors(0)
    , m_pages(pages)
    , m_blockIndex(-1)
{
    window.clear();
    parse(initialFile);
}

void Assembler::dumpLex(const Lex& l)
{
    // Dump the output
    const char* typeNames[(int)Lex::Element::Type::KEYWORDS] = {
        "EOF",
        "UNKNOWN",
        "ERROR",

        "NEWLINE",
        "SYMBOL",
        "INTEGER",
        "STRING",
        "CHAR",

        "COMMA",
        "OPEN-PAREN",
        "CLOSE-PAREN",
        "DOLLAR",
        "PLUS",
        "MINUS",
        "COLON",
        "LOGIC-OR",
        "LOGIC-AND",
        "LOGIC-XOR",
        "SHIFT-LEFT",
        "SHIFT-RIGHT",
        "TILDE",
        "MULTIPLY",
        "DIVIDE",
        "MOD",
    };

    using T = Lex::Element::Type;

    for (const auto& el : l.elements())
    {
        string line;

        //
        // Draw first line describing it
        //
        if ((int)el.m_type < (int)T::KEYWORDS)
        {
            line = stringFormat("{0}: {1}", el.m_position.m_line, typeNames[(int)el.m_type]);

            switch (el.m_type)
            {
            case T::Symbol:
                line += stringFormat(": {0}", m_symbols.get(el.m_symbol));
                break;

            case T::Integer:
                line += stringFormat(": {0}", el.m_integer);
                break;

            case T::String:
            case T::Char:
                line += string(": \"") + string(el.m_s0, el.m_s1) + "\"";
                break;

            default:
                break;
            }
        }
        else
        {
            line = stringFormat("{0}: {1}", el.m_position.m_line, l.getKeywordString(el.m_type));
        }
        output(line);

        //
        // Output source/marker
        //
        if (el.m_type > T::EndOfFile && el.m_type != T::KEYWORDS)
        {
            int x = el.m_position.m_col - 1;
            int len = (int)(el.m_s1 - el.m_s0);

            // Print line that token resides in
            line.clear();
            const vector<u8>& file = l.getFile();
            for (const i8* p = (const i8 *)(file.data() + el.m_position.m_lineOffset);
                (p < (const i8 *)(file.data() + file.size()) && (*p != '\r') && (*p != '\n'));
                ++p)
            {
                line += *p;
            }
            output(line);
            line.clear();

            // Print the marker
            for (int j = 0; j < x; ++j) line += ' ';
            line += '^';
            for (int j = 0; j < len - 1; ++j) line += '~';
            output(line);
        }
        output("");
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Parser
//----------------------------------------------------------------------------------------------------------------------


void Assembler::parse(std::string initialFile)
{
    //
    // Initialise parser
    //
    m_blocks.clear();
    m_symbolTable.clear();
    m_blockIndex = -1;
    m_address = addressUp(0x8000);

    //
    // Lexical Analysis
    //
    if (initialFile.empty())
    {
        output("!Error: Cannot assemble a file if it has not been saved.");
        addError();
    }
    else
    {
        auto it = m_sessions.find(initialFile);
        if (it == m_sessions.end())
        {
            m_sessions[initialFile] = Lex();
            m_sessions[initialFile].parse(*this, initialFile);

#if NX_DEBUG_LOG_LEX
            dumpLex(m_sessions[initialFile]);
#endif // NX_DEBUG_LOG_LEX

            //
            // Assemble
            //
            if (!numErrors()) assemble(m_sessions[initialFile], m_sessions[initialFile].elements());
        }
    }

    m_assemblerWindow.output("");
    if (numErrors())
    {
        m_assemblerWindow.output(stringFormat("!Assembler error(s): {0}", numErrors()));
    }
    else
    {
        m_assemblerWindow.output(stringFormat("*\"{0}\" assembled ok!", initialFile));
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Utility
//----------------------------------------------------------------------------------------------------------------------

void Assembler::output(const std::string& msg)
{
    m_assemblerWindow.output(msg);
}

void Assembler::addError()
{
    ++m_numErrors;
}

i64 Assembler::getSymbol(const u8* start, const u8* end)
{
    return m_symbols.add((const char *)start, (const char *)end);
}

void Assembler::error(const Lex& l, const Lex::Element& el, const string& message)
{
    Lex::Element::Pos start = el.m_position;
    int length = int(el.m_s1 - el.m_s0);

    // Output the error
    string err = stringFormat("!{0}({1}): {2}", l.getFileName(), start.m_line, message);
    output(err);
    addError();

    // Output the markers
    string line;
    int x = start.m_col - 1;

    // Print line that token resides in
    const vector<u8>& file = l.getFile();
    for (const i8* p = (const i8 *)(file.data() + start.m_lineOffset);
        (p < (const i8 *)(file.data() + file.size()) && (*p != '\r') && (*p != '\n'));
        ++p)
    {
        line += *p;
    }
    output(line);
    line = "!";

    // Print the marker
    for (int j = 0; j < x; ++j) line += ' ';
    line += '^';
    for (int j = 0; j < length - 1; ++j) line += '~';
    output(line);
}

Assembler::Address Assembler::addressUp(u16 addr)
{
    int slot = int(addr >> 14);
    return { u8(slot), u16(addr & 0x3fff) };
}

Assembler::Address Assembler::incAddress(Address addr, int n)
{
    u64 fullAddress = getFullAddress(addr);
    fullAddress += n;
    return getPagedAddress(fullAddress);
}

Assembler::Block& Assembler::openBlock()
{
    Address addr = getAddress();
    Block blk(addr);
    auto it = m_blocks.insert(upper_bound(m_blocks.begin(), m_blocks.end(), blk), blk);
    m_blockIndex = it - m_blocks.begin();
    return *it;
}

Assembler::Address Assembler::getAddress()
{
    return m_address;
}

Assembler::Ref Assembler::getRef(const string& fileName, int line)
{
    u64 addr = getFullAddress(m_address);
    u64 offset = addr - getFullAddress(block().m_address);
    i64 fn = m_files.add(fileName.c_str());
    return Ref{ m_blockIndex, offset, fn, line };
}

Assembler::Block& Assembler::block()
{
    return m_blocks[m_blockIndex];
}

//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

void Assembler::assemble(const Lex& l, const vector<Lex::Element>& elems)
{
    const Lex::Element* e = elems.data();

    using T = Lex::Element::Type;

    while (e->m_type != T::EndOfFile)
    {
        if (e->m_type == T::Symbol || e->m_type > T::KEYWORDS)
        {
            // Lines start with either a symbol or keyword.
            const Lex::Element* start = e;
            do {
                ++e;
            } while (e->m_type != T::Newline && e->m_type != T::EndOfFile);

            // We convert the line into an instruction and then assemble it.
            assembleLine(l, start, e);
        }
        else if (e->m_type != T::Newline)
        {
            error(l, *e, "Invalid opcode.");
            while (e->m_type != T::Newline && e->m_type != T::EndOfFile) ++e;
        }
        else
        {
            ++e;
        }
    }
}

void Assembler::assembleLine(const Lex& l, const Lex::Element* e, const Lex::Element* end)
{
    using T = Lex::Element::Type;

    //
    // Check for symbols (i.e. labels)
    //
    if (e[0].m_type == T::Symbol)
    {
        // Check to see if we already have the symbol.
        i64 sym = e[0].m_symbol;
        auto it = m_symbolTable.find(sym);
        if (it != m_symbolTable.end())
        {
            // We already have this symbol
            error(l, *e, "Symbol already defined.");
            output(stringFormat("Original @ {0}({1})", m_files.get(it->second.m_fileName), it->second.m_line));
            return;
        }

        // Start a new block.  And add the symbol
        openBlock();
        m_symbolTable[sym] = getRef(l.getFileName(), e[0].m_position.m_line);

        if (e[1].m_type == T::Colon) ++e;
        ++e;
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
