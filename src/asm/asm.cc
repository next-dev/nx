//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/overlay_asm.h>
#include <emulator/nxfile.h>
#include <emulator/spectrum.h>
#include <utils/format.h>

#define NX_DEBUG_LOG_LEX    (0)

//----------------------------------------------------------------------------------------------------------------------
// MemoryMap::Byte
//----------------------------------------------------------------------------------------------------------------------

MemoryMap::Byte::Byte()
    : m_pass(0)
    , m_byte(0)
{

}

bool MemoryMap::Byte::poke(u8 b, u8 currentPass)
{
    if (currentPass > m_pass)
    {
        m_byte = b;
        return true;
    }
    else
    {
        return false;
    }
}

bool MemoryMap::Byte::written() const
{
    return m_pass > 0;
}

//----------------------------------------------------------------------------------------------------------------------
// MemoryMap
//----------------------------------------------------------------------------------------------------------------------

MemoryMap::MemoryMap(Spectrum& speccy)
    : m_model(speccy.getModel())
    , m_currentPass(0)
{
    size_t ramSize;
    switch (m_model)
    {
    case Model::ZX48:
        ramSize = KB(48);
        m_slots = { 0, 1, 2, 3 };
        break;

    case Model::ZX128:
    case Model::ZXPlus2:
        ramSize = KB(128);
        m_slots = { 0, 5, 2, 0 };
        break;
    }

    for (int i = 0; i < kNumSlots; ++i)
    {
        m_slots[i] = speccy.getPage(i);
    }

    m_memory.resize(ramSize);
    addZ80Range(0x8000, 0xffff);
}

void MemoryMap::setPass(int pass)
{
    assert(pass > 0);
    m_currentPass = pass;
}

void MemoryMap::resetRange()
{
    m_addresses.clear();
}

void MemoryMap::addRange(Address start, Address end)
{
    assert(end > start);
    assert(m_model != Model::ZX48);

    m_addresses.reserve(m_addresses.size() + (end - start));
    for (Address i = start; i < end; ++i)
    {
        m_addresses.push_back(i);
    }
}

void MemoryMap::addZ80Range(u16 start, u16 end)
{
    assert(start >= 0x4000);
    assert(end > start);
    m_addresses.reserve(m_addresses.size() + (end - start));

    for (u16 i = start; i < end; ++i)
    {
        u16 slot = i / kPageSize;
        u16 offset = i % kPageSize;

        assert(slot >= 0 && slot < m_slots.size());

        Address addr = m_slots[slot] * kPageSize + offset;

        m_addresses.push_back(addr);
    }
}

bool MemoryMap::poke8(int address, u8 byte)
{
    return m_memory[m_addresses[address]].poke(byte, m_currentPass);
}

bool MemoryMap::poke16(int address, u16 word)
{
    if (!m_memory[m_addresses[address]].poke(word % 256, m_currentPass)) return false;
    return m_memory[m_addresses[address + 1]].poke(word / 256, m_currentPass);
}

void MemoryMap::upload(Spectrum& speccy)
{
    // #todo: uploading memory to speccy
}

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Assembler::Assembler(AssemblerWindow& window, Spectrum& speccy, const vector<u8>& data, string sourceName)
    : m_assemblerWindow(window)
    , m_speccy(speccy)
    , m_numErrors(0)
    , m_mmap(speccy)
    , m_address(0)
{
    window.clear();
    startAssembly(data, sourceName);
}

//----------------------------------------------------------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------------------------------------------------------

void Assembler::dumpLex(const Lex& l)
{
    // Dump the output
    const char* typeNames[(int)Lex::Element::Type::_KEYWORDS] = {
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
        if ((int)el.m_type < (int)T::_KEYWORDS)
        {
            line = stringFormat("{0}: {1}", el.m_position.m_line, typeNames[(int)el.m_type]);

            switch (el.m_type)
            {
            case T::Symbol:
                line += stringFormat(": {0}", m_lexSymbols.get(el.m_symbol));
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
        if (el.m_type > T::EndOfFile && el.m_type != T::_KEYWORDS)
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

void Assembler::dumpSymbolTable()
{
    output("");
    output("----------------------------------------");
    output("Symbol table:");
    output("Symbol           Address");
    output("----------------------------------------");

    struct Symbols
    {
        string      m_symbol;
        string      m_address;

        Symbols() {}
        Symbols(string symbol, string address) : m_symbol(symbol), m_address(address) {}
    };
    vector<Symbols> symbols;

    // Construct the output data
    for (const auto& symPair : m_symbolTable)
    {
        string symbol = (const char *)m_lexSymbols.get(symPair.first);

        int page = symPair.second.m_addr / kPageSize;
        int offset = symPair.second.m_addr % kPageSize;
        string addressString;

        for (int slot = 0; slot < kNumSlots; ++slot)
        {
            int loadedPage = m_speccy.getPage(slot);
            if (page == loadedPage)
            {
                // Show string as Z80 address
                addressString = stringFormat("${0}", hexWord(u16(slot * KB(16) + offset)));
                break;
            }
        }
        if (addressString.empty())
        {
            addressString = stringFormat("${0}:{1}", hexWord(u16(page)), hexWord(u16(offset)));
        }

        symbols.emplace_back(symbol.substr(0, min(symbol.size(), size_t(16))), addressString);
    }

    // Sort the data in ASCII order for symbols.
    sort(symbols.begin(), symbols.end(), [](const Symbols& s1, const Symbols& s2) { return s1.m_symbol < s2.m_symbol;  });

    // Output the symbols
    for (const auto& sym : symbols)
    {
        string line = sym.m_symbol;
        for (size_t i = line.size(); i < 17; ++i) line += ' ';
        line += sym.m_address;
        output(line);
    }

    output("");
}

//----------------------------------------------------------------------------------------------------------------------
// Parser
//----------------------------------------------------------------------------------------------------------------------

void Assembler::startAssembly(const vector<u8>& data, string sourceName)
{
    assemble(data, sourceName);

    m_assemblerWindow.output("");
    if (numErrors())
    {
        m_assemblerWindow.output(stringFormat("!Assembler error(s): {0}", numErrors()));
    }
    else
    {
        m_assemblerWindow.output(stringFormat("*\"{0}\" assembled ok!", sourceName));
    }
}

void Assembler::assembleFile(string fileName)
{
    const vector<u8> data = NxFile::loadFile(fileName);
    if (!data.empty())
    {
        assemble(data, fileName);
    }
}

void Assembler::assemble(const vector<u8>& data, string sourceName)
{
    //
    // Lexical Analysis
    //
    m_sessions.emplace_back();
    m_sessions.back().parse(*this, std::move(data), sourceName);

#if NX_DEBUG_LOG_LEX
    dumpLex(m_sessions.back());
#endif // NX_DEBUG_LOG_LEX

    //
    // Passes
    //
    // We store the index since passes may add new lexes.
    size_t index = m_sessions.size() - 1;
    pass1(m_sessions[index], m_sessions[index].elements());
    pass2(m_sessions[index], m_sessions[index].elements());

    dumpSymbolTable();
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

//----------------------------------------------------------------------------------------------------------------------
// Symbol table
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::addSymbol(i64 symbol, MemoryMap::Address address)
{
    auto it = m_symbolTable.find(symbol);
    if (it == m_symbolTable.end())
    {
        // We're good.  This is a new symbol
        m_symbolTable[symbol] = { address };
        return true;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Parsing utilties
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::expect(Lex& lex, const Lex::Element* e, const char* format, const Lex::Element** outE /* = nullptr */)
{
    using T = Lex::Element::Type;

    for (char c = *format; c != 0; ++format)
    {
        bool pass = false;
        enum class Mode
        {
            Normal,
            Optional,
            OneOf,
        } mode = Mode::Normal;

        switch (c)
        {
        case '[':
            mode = Mode::Optional;
            pass = true;
            break;

        case ']':
            pass = true;
            break;

        case '{':
            mode = Mode::OneOf;
            pass = true;
            break;

        case '}':
            pass = false;
            break;

        case ',':
            pass = (e->m_type == T::Comma);
            break;

        case '(':
            pass = (e->m_type == T::OpenParen);
            break;

        case ')':
            pass = (e->m_type == T::CloseParen);
            break;

        case '\'':
            pass = (e->m_type == T::AF_);
            break;

        case 'a':
            pass = (e->m_type == T::A);
            break;

        case 'b':
            pass = (e->m_type == T::B);
            break;

        case 'c':
            pass = (e->m_type == T::C);
            break;

        case 'd':
            pass = (e->m_type == T::D);
            break;

        case 'e':
            pass = (e->m_type == T::E);
            break;

        case 'h':
            pass = (e->m_type == T::H);
            break;

        case 'l':
            pass = (e->m_type == T::L);
            break;

        case 'i':
            pass = (e->m_type == T::I);
            break;

        case 'r':
            pass = (e->m_type == T::R);
            break;

        case 'A':
            pass = (e->m_type == T::AF);
            break;

        case 'B':
            pass = (e->m_type == T::BC);
            break;

        case 'D':
            pass = (e->m_type == T::DE);
            break;

        case 'H':
            pass = (e->m_type == T::HL);
            break;

        case 'S':
            pass = (e->m_type == T::SP);
            break;

        case 'X':
            pass = (e->m_type == T::IX);
            break;

        case 'Y':
            pass = (e->m_type == T::IY);
            break;

        case '*':
            pass = expectExpression(lex, e, outE);
            break;

        case '%':
            pass = false;
            if (e->m_type == T::IX || e->m_type == T::IY)
            {
                ++e;
                if (e->m_type == T::Plus || e->m_type == T::Minus)
                {
                    pass = expectExpression(lex, e, &e);
                    --e;
                }
            }
            break;
        }

        if (pass)
        {
            // If we're in a one-of group, we need to skip to the end
            if (mode == Mode::OneOf)
            {
                while (c != '}') c = *format++;
            }
            // If we're in an optional group, we need to skip to the end
            if (mode == Mode::Optional)
            {
                while (c != ']') c = *format++;
            }
            mode = Mode::Normal;
        }
        else
        {
            // If we're in an optional group, it doesn't matter if we pass or not.  Otherwise we fail here.
            if (mode != Mode::Optional)
            {
                return false;
            }
        }

        ++e;
    }

    if (outE) *outE = e;
    return (e->m_type == T::Newline);
}

int Assembler::invalidInstruction(Lex& lex, const Lex::Element* e)
{
    error(lex, *e, "Invalid instruction.");
    return 0;
}

bool Assembler::expectExpression(Lex& lex, const Lex::Element* e, const Lex::Element** outE)
{
    using T = Lex::Element::Type;
    int state = 0;
    int parenDepth = 0;

    for (;;)
    {
        switch (state)
        {
        case 0:
            // Start state
            switch (e->m_type)
            {
            case T::OpenParen:
                ++parenDepth;
                break;

            case T::Dollar:
            case T::Symbol:
            case T::Integer:
            case T::Char:
                state = 1;
                break;

            case T::Plus:
            case T::Minus:
            case T::Tilde:
                state = 2;
                break;

            default:
                // #todo: match keywords and (
                goto expr_failed;
            }
            break;

        case 1:
            // Value has been read
            switch (e->m_type)
            {
                // Binary operators
            case T::Plus:
            case T::Minus:
            case T::LogicOr:
            case T::LogicAnd:
            case T::LogicXor:
            case T::ShiftLeft:
            case T::ShiftRight:
            case T::Multiply:
            case T::Divide:
            case T::Mod:
                state = 0;
                break;

            case T::Comma:
            case T::Newline:
                ++e;
                if (parenDepth != 0) goto expr_failed;
                goto finish_expr;

            case T::CloseParen:
                ++e;
                if (parenDepth-- == 0) goto expr_failed;
                break;

            default:
                goto expr_failed;
            }

        case 2:
            switch (e->m_type)
            {
            case T::Dollar:
            case T::Symbol:
            case T::Integer:
            case T::Char:
                state = 1;
                break;

            default:
                goto expr_failed;
            }
        }

        ++e;
    }

finish_expr:
    if (outE) *outE = e;
    return true;

expr_failed:
    if (outE) *outE = e;
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Pass 1
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::pass1(Lex& lex, const vector<Lex::Element>& elems)
{
    output("Pass 1...");

    using T = Lex::Element::Type;
    const Lex::Element* e = elems.data();
    i64 symbol = 0;
    bool symbolToAdd = false;
    bool buildResult = true;

    while (e->m_type != T::EndOfFile)
    {
        symbol = 0;
        symbolToAdd = false;
        if (e->m_type == T::Symbol)
        {
            // Possible label
            symbol = e->m_symbol;
            if ((++e)->m_type == T::Colon) ++e;
        }

        // Figure out if the next token is an opcode or directive
        if (e->m_type > T::_KEYWORDS && e->m_type < T::_END_OPCODES)
        {
            // It's a possible instruction
            m_address += assembleInstruction1(lex, e);
            symbolToAdd = true;

            while (e->m_type != T::Newline) ++e;
            ++e;
            buildResult = false;
        }
        else if (e->m_type > T::_END_OPCODES && e->m_type < T::_END_DIRECTIVES)
        {
            // It's a possible directive
            error(lex, *e, "Unimplemented directive.");
            while (e->m_type != T::Newline) ++e;
            ++e;
            buildResult = false;
        }
        else if (e->m_type != T::Newline)
        {
            error(lex, *e, "Invalid instruction or directive.");
            while (e->m_type != T::Newline) ++e;
            ++e;
            buildResult = false;
        }
        else
        {
            ++e;
            symbolToAdd = true;
        }

        if (symbolToAdd && symbol)
        {
            if (!addSymbol(symbol, m_mmap.getAddress(m_address)))
            {
                // #todo: Output the original line where it is defined.
                error(lex, *e, "Symbol already defined.");
            }
        }
    }

    return true;
}

#define PARSE(n, format) if (expect(lex, e, (format))) return (n)
#define CHECK_PARSE(n, format) PARSE(n, format); return invalidInstruction(lex, e)

int Assembler::assembleInstruction1(Lex& lex, const Lex::Element* e)
{
    using T = Lex::Element::Type;
    assert(e->m_type > T::_KEYWORDS && e->m_type < T::_END_OPCODES);

    switch (e->m_type)
    {
    case T::NOP:
        ++e;
        CHECK_PARSE(1, "");

    case T::LD:
        return assembleLoad1(lex, ++e);

    default:
        error(lex, *e, "Unimplemented instruction.");
        return 0;
    }
}


int Assembler::assembleLoad1(Lex& lex, const Lex::Element* e)
{
    using T = Lex::Element::Type;
    const Lex::Element* start = e;

    PARSE(1, "{abcdehl},{abcdehl}");    // LD A/B/C/D/E/H/L,A/B/C/D/E/H/L
    PARSE(1, "({BDH}),a");              // LD (BC/DE/HL),A
    PARSE(1, "(H),{bcdehl}");           // LD (HL),B/C/D/E/H/L
    PARSE(1, "{bcdehl},(H)");           // LD B/C/D/E/H/L,(HL)
    PARSE(1, "a,({BDH})");              // LD A,(BC/DE/HL)
    PARSE(2, "{abcdehl},*");            // LD A/B/C/D/E/H/L,nn
    PARSE(3, "{BDHS},*");               // LD BC/DE/HL/SP,nnnn
    PARSE(4, "{XY},*");                 // LD IX/IY,nnnn
    PARSE(2, "(H),*");                  // LD (HL),nn
    PARSE(2, "a,i");                    // LD A,I
    PARSE(2, "i,a");                    // LD I,A
    PARSE(3, "a,(%)");                  // LD A,(IX+nn)
    PARSE(3, "a,(*)");                  // LD A,(nnnn)
    PARSE(3, "(*),a");                  // LD (nnnn),A
    PARSE(4, "(*),{BDHXYS}");           // LD (nnnn),BC/DE/HL/IX/IY/SP
    PARSE(4, "{BDHXYS},(*)");           // LD BC/DE/HL/IX/IY/SP,(nnnn)
    PARSE(3, "(%),{abcdehl}");          // LD (IX/IY+nn),A/B/C/D/E/H/L
    PARSE(3, "{abcdehl},(%)");          // LD A/B/C/D/E/H/L,(IX/IY+nn)
    PARSE(4, "(%),*");                  // LD (IX/IY+nn),nn
    PARSE(1, "S,H");                    // LD SP,HL
    PARSE(2, "S,{XY}");                 // LD SP,IX/IY

    return invalidInstruction(lex, start);
}

//----------------------------------------------------------------------------------------------------------------------
// Pass 2
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::pass2(Lex& lex, const vector<Lex::Element>& elems)
{
    output("Pass 2...");
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
