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
        "DOLLAR",

        "COMMA",
        "OPEN-PAREN",
        "CLOSE-PAREN",
        "COLON",

        "PLUS",
        "MINUS",
        "LOGIC-OR",
        "LOGIC-AND",
        "LOGIC-XOR",
        "SHIFT-LEFT",
        "SHIFT-RIGHT",
        "TILDE",
        "MULTIPLY",
        "DIVIDE",
        "MOD",

        "UNARY_PLUS",
        "UNARY_MINUS",
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
    if (pass1(m_sessions[index], m_sessions[index].elements()))
    {
        if (pass2(m_sessions[index], m_sessions[index].elements()))
        {
            dumpSymbolTable();
        }
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

bool Assembler::addValue(i64 symbol, i64 value)
{
    auto symIt = m_symbolTable.find(symbol);
    bool result = false;
    if (symIt == m_symbolTable.end())
    {
        auto it = m_values.find(symbol);
        if (it == m_values.end())
        {
            m_values[symbol] = value;
            result = true;
        }
    }

    return result;
}

optional<i64> Assembler::lookUpLabel(i64 symbol)
{
    auto it = m_symbolTable.find(symbol);
    return it == m_symbolTable.end() ? optional<i64>{} : it->second.m_addr;
}

optional<i64> Assembler::lookUpValue(i64 symbol)
{
    auto it = m_values.find(symbol);
    return it == m_values.end() ? optional<i64>{} : it->second;
}


//----------------------------------------------------------------------------------------------------------------------
// Parsing utilities
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::expect(Lex& lex, const Lex::Element* e, const char* format, const Lex::Element** outE /* = nullptr */)
{
    using T = Lex::Element::Type;

    enum class Mode
    {
        Normal,
        Optional,
        OneOf,
    } mode = Mode::Normal;

    for (char c = *format; c != 0; c = *(++format))
    {
        bool pass = false;
        switch (c)
        {
        case '[':
            mode = Mode::Optional;
            pass = true;
            break;

        case ']':
            pass = true;
            mode = Mode::Normal;
            break;

        case '{':
            mode = Mode::OneOf;
            pass = false;
            break;

        case '}':
            pass = false;
            mode = Mode::Normal;
            break;

        case ',':
            pass = (e->m_type == T::Comma);
            if (pass) ++e;
            break;

        case '(':
            pass = (e->m_type == T::OpenParen);
            if (pass) ++e;
            break;

        case ')':
            pass = (e->m_type == T::CloseParen);
            if (pass) ++e;
            break;

        case '\'':
            pass = (e->m_type == T::AF_);
            if (pass) ++e;
            break;

        case 'a':
            pass = (e->m_type == T::A);
            if (pass) ++e;
            break;

        case 'b':
            pass = (e->m_type == T::B);
            if (pass) ++e;
            break;

        case 'c':
            pass = (e->m_type == T::C);
            if (pass) ++e;
            break;

        case 'd':
            pass = (e->m_type == T::D);
            if (pass) ++e;
            break;

        case 'e':
            pass = (e->m_type == T::E);
            if (pass) ++e;
            break;

        case 'h':
            pass = (e->m_type == T::H);
            if (pass) ++e;
            break;

        case 'l':
            pass = (e->m_type == T::L);
            if (pass) ++e;
            break;

        case 'i':
            pass = (e->m_type == T::I);
            if (pass) ++e;
            break;

        case 'r':
            pass = (e->m_type == T::R);
            if (pass) ++e;
            break;

        case 'A':
            pass = (e->m_type == T::AF);
            if (pass) ++e;
            break;

        case 'B':
            pass = (e->m_type == T::BC);
            if (pass) ++e;
            break;

        case 'D':
            pass = (e->m_type == T::DE);
            if (pass) ++e;
            break;

        case 'H':
            pass = (e->m_type == T::HL);
            if (pass) ++e;
            break;

        case 'S':
            pass = (e->m_type == T::SP);
            if (pass) ++e;
            break;

        case 'X':
            pass = (e->m_type == T::IX);
            if (pass) ++e;
            break;

        case 'Y':
            pass = (e->m_type == T::IY);
            if (pass) ++e;
            break;

        case 'f':
            pass =
                (e->m_type == T::NZ ||
                 e->m_type == T::Z ||
                 e->m_type == T::NC ||
                 e->m_type == T::C);
            if (pass) ++e;
            break;

        case 'F':
            pass =
                (e->m_type == T::NZ ||
                 e->m_type == T::Z ||
                 e->m_type == T::NC ||
                 e->m_type == T::C ||
                 e->m_type == T::PO ||
                 e->m_type == T::PE ||
                 e->m_type == T::P ||
                 e->m_type == T::M);
            if (pass) ++e;
            break;

        case '*':
            {
                const Lex::Element* ee;
                pass = expectExpression(lex, e, &ee);
                e = pass ? ee : e + 1;
            }
            break;

        case '%':
            pass = false;
            if (e->m_type == T::IX || e->m_type == T::IY)
            {
                ++e;
                if (e->m_type == T::Plus || e->m_type == T::Minus)
                {
                    pass = expectExpression(lex, e, &e);
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
                --format;
            }
            // If we're in an optional group, we need to skip to the end
            if (mode == Mode::Optional)
            {
                while (c != ']') c = *format++;
                --format;
            }
            mode = Mode::Normal;
        }
        else
        {
            // If we're in an optional group, it doesn't matter if we pass or not.  Otherwise we fail here.
            if (mode == Mode::Normal)
            {
                return false;
            }
        }
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
                if (parenDepth != 0) goto expr_failed;
                goto finish_expr;

            case T::CloseParen:
                if (parenDepth > 0)
                {
                    --parenDepth;
                }
                else
                {
                    goto finish_expr;
                }
                break;

            default:
                goto expr_failed;
            }
            break;

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
            break;
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

#define FAIL(msg)                               \
    do {                                        \
    error(lex, *e, (msg));                      \
    while (e->m_type != T::Newline) ++e;        \
    ++e;                                        \
    buildResult = false;                        \
    } while (0)

bool Assembler::pass1(Lex& lex, const vector<Lex::Element>& elems)
{
    output("Pass 1...");

    using T = Lex::Element::Type;
    const Lex::Element* e = elems.data();
    i64 symbol = 0;
    bool symbolToAdd = false;
    bool buildResult = true;
    int symAddress = 0;

    while (e->m_type != T::EndOfFile)
    {
        // Set if symbol is found
        symbol = 0;
        // Set to true if this is a symbol to add to the symbol table (and symbol must be non-zero).
        symbolToAdd = false;

        if (e->m_type == T::Symbol)
        {
            // Possible label
            symbol = e->m_symbol;
            symAddress = m_address;
            if ((++e)->m_type == T::Colon) ++e;
        }

        // Figure out if the next token is an opcode or directive
        if (e->m_type > T::_KEYWORDS && e->m_type < T::_END_OPCODES)
        {
            // It's a possible instruction
            const Lex::Element* outE;
            m_address += assembleInstruction1(lex, e, &outE);
            symbolToAdd = true;

            e = outE;
            buildResult = (e->m_type == T::Newline);
            if (!buildResult)
            {
                while (e->m_type != T::Newline) ++e;
            }
            ++e;
        }
        else if (e->m_type > T::_END_OPCODES && e->m_type < T::_END_DIRECTIVES)
        {
            // It's a possible directive
            switch (e->m_type)
            {
            case T::ORG:
                // #todo: Allow expressions for ORG statements (requires expression evaluation)
                if ((++e)->m_type == T::Integer)
                {
                    if (e->m_integer >= 0x4000 && e->m_integer <= 0xffff)
                    {
                        u16 addr = u16(e->m_integer);
                        if ((++e)->m_type == T::Newline)
                        {
                            m_mmap.resetRange();
                            m_mmap.addZ80Range(addr, 0xffff);
                            m_address = 0;
                            ++e;
                        }
                        else
                        {
                            FAIL("Invalid syntax for ORG directive.  Extraneous tokens found.");
                        }
                    }
                    else
                    {
                        FAIL("ORG address out of range.  Must be between $4000-$ffff.");
                    }
                }
                else
                {
                    FAIL("Invalid syntax for ORG directive.  Expected an address.");
                }
                break;

            case T::EQU:
                if (symbol)
                {
                    if (!expect(lex, ++e, "*", &e))
                    {
                        FAIL("Invalid syntax for EQU directive.");
                    }
                }
                else
                {
                    FAIL("Missing label in EQU directive.");
                }
                break;

            default:
                FAIL("Unimplemented directive.");
            }
        }
        else if (e->m_type != T::Newline)
        {
            FAIL("Invalid instruction or directive.");
        }
        else
        {
            ++e;
            symbolToAdd = true;
        }

        if (symbolToAdd && symbol)
        {
            if (m_mmap.isValidAddress(symAddress))
            {
                if (!addSymbol(symbol, m_mmap.getAddress(symAddress)))
                {
                    // #todo: Output the original line where it is defined.
                    error(lex, *e, "Symbol already defined.");
                    buildResult = false;
                }
            }
            else
            {
                error(lex, *e, "Address space overrun.  There is not enough space to assemble in this area section.");
            }
        }
    }

    return buildResult;
}

#undef FAIL

#define PARSE(n, format) if (expect(lex, e, (format), outE)) return (n)
#define CHECK_PARSE(n, format) PARSE(n, format); return invalidInstruction(lex, e)

int Assembler::assembleInstruction1(Lex& lex, const Lex::Element* e, const Lex::Element** outE)
{
    using T = Lex::Element::Type;
    assert(e->m_type > T::_KEYWORDS && e->m_type < T::_END_OPCODES);

    switch (e->m_type)
    {
    case T::ADC:
        ++e;
        PARSE(1, "a,{abcdehl}");
        PARSE(2, "a,*");
        PARSE(1, "a,(H)");
        PARSE(2, "H,{BDHS}");
        CHECK_PARSE(3, "a,(%)");
        break;

    case T::ADD:
        ++e;
        PARSE(1, "a,{abcdehl}");
        PARSE(1, "a,(H)");
        PARSE(2, "a,*");
        PARSE(1, "H,{BDHS}");
        PARSE(2, "X,{BDXS}");
        PARSE(2, "Y,{BDYS}");
        CHECK_PARSE(3, "a,(%)");
        break;

    case T::BIT:
    case T::RES:
    case T::SET:
        ++e;
        PARSE(2, "*,{abcdehl}");
        PARSE(2, "*,(H)");
        CHECK_PARSE(4, "*,(%)");
        break;

    case T::AND:
    case T::CP:
    case T::OR:
    case T::XOR:
        ++e;
        PARSE(1, "a,{abcdehl}");
        PARSE(1, "{abcdehl}");
        PARSE(1, "a,(H)");
        PARSE(1, "(H)");
        PARSE(2, "*");
        PARSE(2, "a,*");
        PARSE(3, "a,(%)");
        CHECK_PARSE(3, "(%)");
        break;

    case T::CALL:
    case T::JP:
        ++e;
        PARSE(1, "(H)");
        PARSE(2, "({XY})");
        PARSE(3, "*");
        CHECK_PARSE(3, "F,*");
        break;

    case T::CCF:
    case T::CPD:
    case T::CPDR:
    case T::CPI:
    case T::CPIR:
    case T::CPL:
    case T::DAA:
    case T::DI:
    case T::EI:
    case T::EXX:
    case T::HALT:
    case T::IND:
    case T::INDR:
    case T::INI:
    case T::INIR:
    case T::LDD:
    case T::LDDR:
    case T::LDI:
    case T::LDIR:
    case T::NEG:
    case T::NOP:
    case T::OTDR:
    case T::OTIR:
    case T::OUTD:
    case T::OUTI:
    case T::RETI:
    case T::RETN:
    case T::RLA:
    case T::RLD:
    case T::RRA:
    case T::RRD:
    case T::RLCA:
    case T::RRCA:
    case T::SCF:
        ++e;
        CHECK_PARSE(1, "");

    case T::DEC:
    case T::INC:
        ++e;
        PARSE(1, "{abcdehlBDHS}");
        PARSE(1, "(H)");
        PARSE(2, "{XY}");
        CHECK_PARSE(3, "(%)");
        break;

    case T::DJNZ:
        ++e;
        CHECK_PARSE(2, "*");
        break;

    case T::EX:
        ++e;
        PARSE(1, "A,'");
        PARSE(1, "D,H");
        PARSE(1, "(S),H");
        CHECK_PARSE(2, "(S),{XY}");
        break;

    case T::IM:
        ++e;
        CHECK_PARSE(2, "*");
        break;

    case T::IN:
        ++e;
        PARSE(2, "{abcdehl},(c)");
        CHECK_PARSE(2, "a,(*)");
        break;

    case T::JR:
        ++e;
        PARSE(2, "*");
        CHECK_PARSE(2, "f,*");
        break;

    case T::LD:
        return assembleLoad1(lex, ++e, outE);

    case T::OUT:
        ++e;
        PARSE(2, "(c),{abcdehl}");
        CHECK_PARSE(2, "(*),a");
        break;

    case T::POP:
    case T::PUSH:
        ++e;
        PARSE(1, "{ABDH}");
        CHECK_PARSE(2, "{XY}");
        break;

    case T::RET:
        ++e;
        PARSE(1, "");
        CHECK_PARSE(1, "F");
        break;

    case T::RLC:
    case T::RL:
    case T::RR:
    case T::RRC:
    case T::SLA:
    case T::SRA:
        ++e;
        PARSE(2, "{abcdehl}");
        PARSE(2, "(H)");
        CHECK_PARSE(4, "(%)");
        break;

    case T::SLL:
    case T::SRL:
        ++e;
        PARSE(2, "{abcdehl}");
        CHECK_PARSE(2, "(H)");
        break;

    case T::RST:
        ++e;
        CHECK_PARSE(1, "*");
        break;

    case T::SBC:
        ++e;
        PARSE(1, "a,{abcdehl}");
        PARSE(1, "{abcdehl}");
        PARSE(1, "a,(H)");
        PARSE(1, "(H)");
        PARSE(2, "a,*");
        PARSE(2, "*");
        PARSE(2, "H,{BDHS}");
        PARSE(3, "a,(%)");
        CHECK_PARSE(3, "(%)");
        break;

    case T::SUB:
        ++e;
        PARSE(1, "a,{abcdehl}");
        PARSE(1, "{abcdehl}");
        PARSE(1, "a,(H)");
        PARSE(1, "(H)");
        PARSE(2, "a,*");
        PARSE(2, "*");
        PARSE(3, "a,(%)");
        CHECK_PARSE(3, "(%)");
        break;

    default:
        error(lex, *e, "Unimplemented instruction.");
        return 0;
    }
}


int Assembler::assembleLoad1(Lex& lex, const Lex::Element* e, const Lex::Element** outE)
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
    PARSE(3, "(*),{aH}");               // LD (nnnn),A/HL
    PARSE(4, "(*),{BDXYS}");            // LD (nnnn),BC/DE/IX/IY/SP
    PARSE(4, "{BDHXYS},(*)");           // LD BC/DE/HL/IX/IY/SP,(nnnn)
    PARSE(3, "(%),{abcdehl}");          // LD (IX/IY+nn),A/B/C/D/E/H/L
    PARSE(3, "{abcdehl},(%)");          // LD A/B/C/D/E/H/L,(IX/IY+nn)
    PARSE(4, "(%),*");                  // LD (IX/IY+nn),nn
    PARSE(1, "S,H");                    // LD SP,HL
    PARSE(2, "S,{XY}");                 // LD SP,IX/IY

    return invalidInstruction(lex, start);
}

#undef PARSE
#undef CHECK_PARSE

//----------------------------------------------------------------------------------------------------------------------
// Expression evaluator
//----------------------------------------------------------------------------------------------------------------------

Assembler::Expression::Expression()
{

}

void Assembler::Expression::addValue(ValueType type, i64 value, const Lex::Element* e)
{
    assert(type == ValueType::Integer ||
           type == ValueType::Symbol ||
           type == ValueType::Char ||
           type == ValueType::Dollar);
    m_queue.emplace_back(type, value, e);
}

void Assembler::Expression::addUnaryOp(Lex::Element::Type op, const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::UnaryOp, (i64)op, e);
}

void Assembler::Expression::addBinaryOp(Lex::Element::Type op, const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::BinaryOp, (i64)op, e);
}

void Assembler::Expression::addOpen(const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::OpenParen, 0, e);
}

void Assembler::Expression::addClose(const Lex::Element* e)
{
    m_queue.emplace_back(ValueType::CloseParen, 0, e);
}

bool Assembler::Expression::eval(Assembler& assembler, Lex& lex, MemoryMap::Address currentAddress)
{
    using T = Lex::Element::Type;

    //
    // Step 1 - convert to reverse polish notation using the Shunting Yard algorithm
    //
    vector<Value>   output;
    vector<Value>   opStack;

    enum class Assoc
    {
        Left,
        Right,
    };

    struct OpInfo
    {
        int     level;
        Assoc   assoc;
    };

    // Operator precedence:
    //
    //      0:  - + ~ (unary ops)
    //      1:  * / %
    //      2:  + -
    //      3:  << >>
    //      4:  &
    //      5:  | ^

    OpInfo opInfo[] = {
        { 2, Assoc::Left },         // Plus
        { 2, Assoc::Left },         // Minus
        { 5, Assoc::Left },         // LogicOr
        { 4, Assoc::Left },         // LogicAnd
        { 5, Assoc::Left },         // LogicXor
        { 3, Assoc::Left },         // ShiftLeft,
        { 3, Assoc::Left },         // ShiftRight,
        { 0, Assoc::Right },        // Unary tilde
        { 1, Assoc::Left },         // Multiply
        { 1, Assoc::Left },         // Divide
        { 1, Assoc::Left },         // Mod
        { 0, Assoc::Right },        // Unary plus
        { 0, Assoc::Right },        // Unary minus
    };

    for (const auto& v : m_queue)
    {
        switch (v.type)
        {
        case ValueType::UnaryOp:
        case ValueType::BinaryOp:
            // Operator
            while (
                !opStack.empty() &&
                (((opInfo[(int)v.elem->m_type - (int)T::Plus].assoc == Assoc::Left) &&
                    (opInfo[(int)v.elem->m_type - (int)T::Plus].level == opInfo[(int)opStack.back().elem->m_type - (int)T::Plus].level)) ||
                    ((opInfo[(int)v.elem->m_type - (int)T::Plus].level > opInfo[(int)opStack.back().elem->m_type - (int)T::Plus].level))))
            {
                output.emplace_back(opStack.back());
                opStack.pop_back();
            }
            // continue...

        case ValueType::OpenParen:
            opStack.emplace_back(v);
            break;

        case ValueType::Integer:
        case ValueType::Symbol:
        case ValueType::Char:
        case ValueType::Dollar:
            output.emplace_back(v);
            break;

        case ValueType::CloseParen:
            while (opStack.back().type != ValueType::OpenParen)
            {
                output.emplace_back(opStack.back());
                opStack.pop_back();
            }
            opStack.pop_back();
            break;
        }

    }

    while (!opStack.empty())
    {
        output.emplace_back(opStack.back());
        opStack.pop_back();
    }

    //
    // Step 2 - Execute the RPN expression
    // If this fails, the expression cannot be evaluated yet return false after outputting a message
    //

#define FAIL()                                                          \
    do {                                                                \
        assembler.error(lex, *v.elem, "Syntax error in expression.");   \
        return false;                                                   \
    } while(0)

    vector<i64> stack;
    i64 a, b;

    for (const auto& v : output)
    {
        switch (v.type)
        {
        case ValueType::Integer:
        case ValueType::Char:
            stack.emplace_back(v.value);
            break;

        case ValueType::Symbol:
            {
                optional<i64> value = assembler.lookUpLabel(v.value);
                if (value)
                {
                    stack.emplace_back(*value);
                }
                else
                {
                    value = assembler.lookUpValue(v.value);
                    if (value)
                    {
                        stack.emplace_back(*value);
                    }
                    else
                    {
                        assembler.error(lex, *v.elem, "Unknown symbol.");
                        return false;
                    }
                }
            }
            break;

        case ValueType::Dollar:
            stack.emplace_back(currentAddress);
            break;

        case ValueType::UnaryOp:
            if (stack.size() < 1) FAIL();
            switch ((T)v.value)
            {
            case T::Unary_Plus:
                // Do nothing
                break;

            case T::Unary_Minus:
                stack.back() = -stack.back();
                break;

            case T::Tilde:
                stack.back() = ~stack.back();
                break;

            default:
                FAIL();
            }
            break;

        case ValueType::BinaryOp:
            if (stack.size() < 2) FAIL();
            b = stack.back();   stack.pop_back();
            a = stack.back();   stack.pop_back();
            switch ((T)v.value)
            {
            case T::Plus:       stack.emplace_back(a + b);      break;
            case T::Minus:      stack.emplace_back(a - b);      break;
            case T::LogicOr:    stack.emplace_back(a | b);      break;
            case T::LogicAnd:   stack.emplace_back(a & b);      break;
            case T::LogicXor:   stack.emplace_back(a ^ b);      break;
            case T::ShiftLeft:  stack.emplace_back(a << b);     break;
            case T::ShiftRight: stack.emplace_back(a >> b);     break;
            case T::Multiply:   stack.emplace_back(a * b);      break;
            case T::Divide:     stack.emplace_back(a / b);      break;
            case T::Mod:        stack.emplace_back(a % b);      break;
            default:
                FAIL();
            }
            break;
        }
    }

#undef FAIL

    assert(stack.size() == 1);
    m_result = stack[0];
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
// Pass 2
//----------------------------------------------------------------------------------------------------------------------

#define FAIL(msg)                                   \
    do {                                            \
        error(lex, *e, (msg));                      \
        while (e->m_type != T::Newline) ++e;        \
        ++e;                                        \
        buildResult = false;                        \
    } while (0)

bool Assembler::pass2(Lex& lex, const vector<Lex::Element>& elems)
{
    output("Pass 2...");

    using T = Lex::Element::Type;
    const Lex::Element* e = elems.data();
    i64 symbol = 0;
    const Lex::Element* symE = 0;
    bool buildResult = true;
    m_mmap.resetRange();
    m_mmap.addZ80Range(0x8000, 0xffff);
    m_address = 0;

    while (e->m_type != T::EndOfFile)
    {
        if (e->m_type == T::Symbol)
        {
            symbol = e->m_symbol;
            symE = e;
            if ((++e)->m_type == T::Colon) ++e;
        }

        // Figure out if the next token is an opcode or directive
        if (e->m_type > T::_KEYWORDS && e->m_type < T::_END_OPCODES)
        {
            // It's an instruction. The syntax has already been checked.  We don't even have to worry about
            // address ranges.
#if _DEBUG
            int count = assembleInstruction1(lex, e, nullptr);
            int oldAddress = m_address;
#endif
            const Lex::Element* outE = assembleInstruction2(lex, e);
#if _DEBUG
            assert(!count || ((oldAddress + count) == m_address));
#endif
            if (!outE)
            {
                buildResult = false;
                while (e->m_type != T::Newline) ++e;
            }
            else
            {
                e = outE;
            }
        }
        else if (e->m_type > T::_END_OPCODES && e->m_type < T::_END_DIRECTIVES)
        {
            switch (e->m_type)
            {
            case T::ORG:
                buildResult = doOrg(lex, ++e);
                break;

            case T::EQU:
                buildResult = doEqu(lex, symbol, ++e);
                break;

            default:
                FAIL("Unimplemented directive");
            }
        }
        else
        {
            ++e;
        }
    }

    return buildResult;
}

#undef FAIL



const Lex::Element* Assembler::assembleInstruction2(Lex& lex, const Lex::Element* e)
{
    using T = Lex::Element::Type;

    // Process the instruction and break it down to the constituent parts.
    Operand srcOp, dstOp;
    T opCode;

    srcOp.type = OperandType::None;
    dstOp.type = OperandType::None;

    //
    // Step 1 - Get the opcode
    //
    opCode = e->m_type;
    const Lex::Element* s = e;
    const Lex::Element* srcE = nullptr;
    ++e;

    //
    // Step 2 - Get destination operand (if exists)
    //
    if (e->m_type == T::Newline)
    {
        // Advance past newline
        ++e;
    }
    else
    {
        if (!buildOperand(lex, e, dstOp))
        {
            return 0;
        }

        if ((e++)->m_type == T::Comma)
        {
            // Step 3 - Get source operand (if exists)
            srcE = e;
            if (!buildOperand(lex, e, srcOp))
            {
                return 0;
            }
        }
        else
        {
            // Advance past comma
            ++e;
        }
    }

    //
    // Step 4 - Assemble!
    //

#define UNDEFINED()                             \
    error(lex, *s, "Unimplemented opcode.");    \
    return false;

#define CHECK8()                                                      \
    if (srcOp.expr.result() < 0 || srcOp.expr.result() > 255) {               \
        error(lex, *srcE, "Expression out of range.  Must be 0-255.");  \
        return false;                                                   \
    }

    switch (opCode)
    {
    case T::LD:
        switch (dstOp.type)
        {
        case OperandType::A:
        case OperandType::B:
        case OperandType::C:
        case OperandType::D:
        case OperandType::E:
        case OperandType::H:
        case OperandType::L:
        case OperandType::Address_HL:
            switch (srcOp.type)
            {
            case OperandType::Expression:       // LD R,nn
                CHECK8();
                emitXYZ(0, r(dstOp.type), 6);
                emit8(u8(srcOp.expr.result()));
                break;

            default: UNDEFINED();
            }
            break;

        default: UNDEFINED();
        } // dstOp.type
        break;

    case T::RET:
        emit8(0xc9);
        break;

    default: UNDEFINED();
    }

#undef UNDEFINED
#undef CHECK8

    return e;
}

bool Assembler::buildOperand(Lex& lex, const Lex::Element*& e, Operand& op)
{
    using T = Lex::Element::Type;

    switch ((e)->m_type)
    {
    case T::Symbol:
    case T::Integer:
    case T::Char:
    case T::Dollar:
    case T::Plus:
    case T::Minus:
    case T::Tilde:
        // Start of an expression.
        op.expr = buildExpression(e);
        if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
        op.type = OperandType::Expression;
        break;

    case T::OpenParen:
        // Addressed expression or (HL) or (IX/Y...)
        ++e;
        switch (e->m_type)
        {
        case T::HL:
            op.type = OperandType::Address_HL;
            break;

        case T::IX:
            op.expr = buildExpression(e);
            if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
            op.type = OperandType::IX_Expression;
            break;

        case T::IY:
            op.expr = buildExpression(e);
            if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
            op.type = OperandType::IY_Expression;
            break;

        default:
            // Must be an address expression
            op.type = OperandType::AddressedExpression;
            --e; // decreased to include token in expression later on
            break;
        }
        if (e->m_type != T::HL)
        {
            op.expr = buildExpression(e);
            if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
            op.type = OperandType::AddressedExpression;
        }

        assert(e->m_type == T::CloseParen);
        ++e;
        break;

    case T::A:      op.type = OperandType::A;           ++e; break;
    case T::AF:     op.type = OperandType::AF;          ++e; break;
    case T::AF_:    op.type = OperandType::AF_;         ++e; break;
    case T::B:      op.type = OperandType::B;           ++e; break;
    case T::BC:     op.type = OperandType::BC;          ++e; break;
    case T::C:      op.type = OperandType::C;           ++e; break;
    case T::D:      op.type = OperandType::D;           ++e; break;
    case T::DE:     op.type = OperandType::DE;          ++e; break;
    case T::E:      op.type = OperandType::E;           ++e; break;
    case T::H:      op.type = OperandType::H;           ++e; break;
    case T::HL:     op.type = OperandType::HL;          ++e; break;
    case T::I:      op.type = OperandType::I;           ++e; break;
    case T::IX:     op.type = OperandType::IX;          ++e; break;
    case T::IY:     op.type = OperandType::IY;          ++e; break;
    case T::L:      op.type = OperandType::L;           ++e; break;
    case T::M:      op.type = OperandType::M;           ++e; break;
    case T::NC:     op.type = OperandType::NC;          ++e; break;
    case T::NZ:     op.type = OperandType::NZ;          ++e; break;
    case T::P:      op.type = OperandType::P;           ++e; break;
    case T::PE:     op.type = OperandType::PE;          ++e; break;
    case T::PO:     op.type = OperandType::PO;          ++e; break;
    case T::R:      op.type = OperandType::R;           ++e; break;
    case T::SP:     op.type = OperandType::SP;          ++e; break;
    case T::Z:      op.type = OperandType::Z;           ++e; break;

    default:
        // We should never reach here - pass 1 should ensure good syntax.
        assert(0);
    }

    return true;
}

Assembler::Expression Assembler::buildExpression(const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    int parenDepth = 0;
    int state = 0;
    Expression expr;

    for (;;)
    {
        switch (state)
        {
        case 0:
            switch (e->m_type)
            {
            case T::OpenParen:
                expr.addOpen(e);
                ++parenDepth;
                break;

            case T::Dollar:     expr.addValue(Expression::ValueType::Dollar, 0, e);                 state = 1;  break;
            case T::Symbol:     expr.addValue(Expression::ValueType::Symbol, e->m_symbol, e);       state = 1;  break;
            case T::Integer:    expr.addValue(Expression::ValueType::Integer, e->m_integer, e);     state = 1;  break;
            case T::Char:       expr.addValue(Expression::ValueType::Char, e->m_integer, e);        state = 1;  break;

            case T::Plus:       expr.addUnaryOp(Lex::Element::Type::Unary_Plus, e);                 state = 2;  break;
            case T::Minus:      expr.addUnaryOp(Lex::Element::Type::Unary_Minus, e);                state = 2;  break;
            case T::Tilde:      expr.addUnaryOp(Lex::Element::Type::Tilde, e);                      state = 2;  break;

            default:
                // Should never reach here!
                assert(0);
            }
            break;

        case 1:
            switch (e->m_type)
            {
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
                expr.addBinaryOp(e->m_type, e);
                state = 0;
                break;

            case T::Comma:
            case T::Newline:
                assert(parenDepth == 0);
                ++e;
                return expr;

            case T::CloseParen:
                if (parenDepth > 0)
                {
                    --parenDepth;
                    expr.addClose(e);
                }
                else
                {
                    return expr;
                }
                break;

            default:
                assert(0);
            }
            break;

        case 2:
            switch (e->m_type)
            {
            case T::Dollar:     expr.addValue(Expression::ValueType::Dollar, 0, e);              state = 1;  break;
            case T::Symbol:     expr.addValue(Expression::ValueType::Symbol, e->m_symbol, e);    state = 1;  break;
            case T::Integer:    expr.addValue(Expression::ValueType::Integer, e->m_integer, e);  state = 1;  break;
            case T::Char:       expr.addValue(Expression::ValueType::Char, e->m_integer, e);     state = 1;  break;
            default:
                assert(0);
            }
        }

        ++e;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Emission utilities
//----------------------------------------------------------------------------------------------------------------------

u8 Assembler::r(OperandType ot) const
{
    switch (ot)
    {
    case OperandType::B:            return 0;
    case OperandType::C:            return 1;
    case OperandType::D:            return 2;
    case OperandType::E:            return 3;
    case OperandType::H:            return 4;
    case OperandType::L:            return 5;
    case OperandType::Address_HL:   return 6;
    case OperandType::A:            return 7;

    default:
        assert(0);
    }
    return 0;
}

u8 Assembler::rp(OperandType ot) const
{
    switch (ot)
    {
    case OperandType::BC:       return 0;
    case OperandType::DE:       return 1;
    case OperandType::HL:       return 2;
    case OperandType::SP:       return 3;

    default:
        assert(0);
    }

    return 0;
}

u8 Assembler::rp2(OperandType ot) const
{
    switch (ot)
    {
    case OperandType::BC:       return 0;
    case OperandType::DE:       return 1;
    case OperandType::HL:       return 2;
    case OperandType::AF:       return 3;

    default:
        assert(0);
    }

    return 0;
}

void Assembler::emit8(u8 b)
{
    m_mmap.poke8(m_address++, b);
}

void Assembler::emit16(u16 w)
{
    m_mmap.poke16(m_address++, w);
}

void Assembler::emitXYZ(u8 x, u8 y, u8 z)
{
    assert(x < 4);
    assert(y < 8);
    assert(z < 8);
    emit8((x << 6) | (y << 3) | z);
}

void Assembler::emitXPQZ(u8 x, u8 p, u8 q, u8 z)
{
    assert(x < 4);
    assert(p < 4);
    assert(q < 2);
    assert(z < 8);
    emit8((x << 6) | (p << 4) | (q << 3) | z);
}

//----------------------------------------------------------------------------------------------------------------------
// Directives
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::doOrg(Lex& lex, const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    Expression addr = buildExpression(e);
    if (!addr.eval(*this, lex, m_address)) return false;
    if (addr.result() < 0 || addr.result() > 0xffff)
    {
        error(lex, *e, "Address out of range.");
        while (e->m_type != T::Newline) ++e;
        ++e;
        return false;
    }
    
    u16 p = u16(addr.result());
    m_mmap.resetRange();
    m_mmap.addZ80Range(p, 0xffff);
    m_address = 0;
    return true;
}

bool Assembler::doEqu(Lex& lex, i64 symbol, const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    Expression expr = buildExpression(e);
    if (!expr.eval(*this, lex, m_address)) return false;

    if (!addValue(symbol, expr.result()))
    {
        error(lex, *e, "Variable name already used.");
        while (e->m_type != T::Newline) ++e;
        ++e;
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
