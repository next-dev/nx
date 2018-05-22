//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/overlay_asm.h>
#include <emulator/nxfile.h>
#include <emulator/spectrum.h>
#include <utils/filename.h>
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
        m_pass = currentPass;
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

void MemoryMap::Byte::clear()
{
    m_byte = 0;
    m_pass = 0;
}

//----------------------------------------------------------------------------------------------------------------------
// MemoryMap
//----------------------------------------------------------------------------------------------------------------------

MemoryMap::MemoryMap(Spectrum& speccy)
    : m_currentPass(0)
{
    clear(speccy);
}

void MemoryMap::clear(Spectrum& speccy)
{
    size_t ramSize = kBankSize * int(speccy.getNumBanks());
    m_memory.resize(ramSize);
    for (auto& b : m_memory)
    {
        b.clear();
    }
    addZ80Range(speccy, Z80MemAddr(0x8000), Z80MemAddr(0xffff));
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

void MemoryMap::addRange(MemAddr start, MemAddr end)
{
    assert(end.offset() > start.offset());
    assert(start.bank() <= end.bank());
    assert(m_model != Model::ZX48);

    m_addresses.reserve(m_addresses.size() + size_t(end - start));
    for (MemAddr i = start; i < end; ++i)
    {
        m_addresses.push_back(i);
    }
}

void MemoryMap::addZ80Range(Spectrum& speccy, Z80MemAddr start, Z80MemAddr end)
{
    MemAddr s = speccy.convertAddress(start);
    MemAddr e = speccy.convertAddress(end);
    addRange(s, e);
}

bool MemoryMap::poke8(int address, u8 byte)
{
    return m_memory[m_addresses[address].index()].poke(byte, m_currentPass);
}

bool MemoryMap::poke16(int address, u16 word)
{
    if (!m_memory[m_addresses[address].index()].poke(word % 256, m_currentPass)) return false;
    return m_memory[m_addresses[address + 1].index()].poke(word / 256, m_currentPass);
}

void MemoryMap::upload(Spectrum& speccy)
{
    MemAddr a;
    for (const auto& b : m_memory)
    {
        if (b.written())
        {
            speccy.memRef(a) = b;
        }
        ++a;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// ExprValue
//----------------------------------------------------------------------------------------------------------------------

ExprValue::ExprValue()
    : m_type(Type::Invalid)
    , m_value(0)
{

}

ExprValue::ExprValue(i64 value)
    : m_type(Type::Integer)
    , m_value(value)
{

}

ExprValue::ExprValue(MemAddr addr)
    : m_type(Type::Address)
    , m_value(addr)
{

}

struct AddExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x + y); }
    ExprValue operator() (MemAddr a, i64 x)         { return ExprValue(a + (int)x); }
    ExprValue operator() (i64 x, MemAddr a)         { return ExprValue(a + (int)x); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct SubExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x - y); }
    ExprValue operator() (MemAddr a, i64 x)         { return ExprValue(a - (int)x); }
    ExprValue operator() (i64 x, MemAddr a)         { return ExprValue(a - (int)x); }
    ExprValue operator() (MemAddr a, MemAddr b)     { return ExprValue(a - b); }
};

struct MulExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x * y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct DivExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x / y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct ModExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x & y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct OrExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x | y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct AndExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x & y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct XorExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x ^ y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct ShiftLeftExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x << y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

struct ShiftRightExpr
{
    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x >> y); }
    ExprValue operator() (MemAddr a, i64 x)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (i64 x, MemAddr a)         { NX_ASSERT(0); return ExprValue(); }
    ExprValue operator() (MemAddr, MemAddr)         { NX_ASSERT(0); return ExprValue(); }
};

ExprValue ExprValue::operator+ (const ExprValue& other) const       { return visit(AddExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator- (const ExprValue& other) const       { return visit(SubExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator* (const ExprValue& other) const       { return visit(MulExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator/ (const ExprValue& other) const       { return visit(DivExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator% (const ExprValue& other) const       { return visit(ModExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator| (const ExprValue& other) const       { return visit(OrExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator& (const ExprValue& other) const       { return visit(AndExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator^ (const ExprValue& other) const       { return visit(XorExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator<< (const ExprValue& other) const      { return visit(ShiftLeftExpr(), m_value, other.m_value); }
ExprValue ExprValue::operator>> (const ExprValue& other) const      { return visit(ShiftRightExpr(), m_value, other.m_value); }

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Assembler::Assembler(AssemblerWindow& window, Spectrum& speccy)
    : m_assemblerWindow(window)
    , m_speccy(speccy)
    , m_mmap(speccy)
    , m_address(0)
{
    window.clear();

    m_options.m_startAddress = MemAddr(Bank(MemGroup::RAM, speccy.getBank(4)), 0);
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
        string addressString = m_speccy.addressName(symPair.second.m_addr);
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

//
// This function is the top-level start to assembly.  This will assemble the source code that appears in the editor
// when you press Ctrl+B or the file given at the command line.  Source name is either an invalid name or the FULL
// filename of the file being assembled.
//
void Assembler::startAssembly(const vector<u8>& data, string sourceName)
{
    //
    // Reset the assembler
    //
    m_assemblerWindow.clear();
    m_sessions.clear();
    m_fileStack.clear();
    m_symbolTable.clear();
    m_values.clear();
    m_lexSymbols.clear();
    m_variables.clear();
    m_mmap.clear(m_speccy);
    m_address = 0;
    m_errors.clear();

    //
    // Set up the assembler
    //
    m_fileStack.emplace_back(sourceName);

    //
    // Assembler
    //
    if (assemble(data, sourceName))
    {
        m_mmap.upload(m_speccy);
    }

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

bool Assembler::assemble(const vector<u8>& data, string sourceName)
{
    //
    // Lexical Analysis
    //
    m_sessions[sourceName] = Lex();
    currentLex().parse(*this, std::move(data), sourceName);

#if NX_DEBUG_LOG_LEX
    dumpLex(m_sessions.back());
#endif // NX_DEBUG_LOG_LEX

    //
    // Passes
    //
    // We store the index since passes may add new lexes.
    output("Pass 1...");
    m_mmap.setPass(1);
    m_mmap.resetRange();
    m_mmap.addZ80Range(m_speccy, Z80MemAddr(0x8000), Z80MemAddr(0xffff));
    m_address = 0;


    if (pass1(currentLex(), currentLex().elements()))
    {
        output("Pass 2...");
        m_mmap.setPass(2);
        m_mmap.resetRange();
        m_mmap.addZ80Range(m_speccy, Z80MemAddr(0x8000), Z80MemAddr(0xffff));
        m_address = 0;

        if (pass2(currentLex(), currentLex().elements()))
        {
            dumpSymbolTable();
            m_fileStack.pop_back();
            return true;
        }
    }
    else
    {
        output("Pass 2 skipped due to errors.");
    }

    m_fileStack.pop_back();
    return false;
}

Path Assembler::findFile(Path givenPath)
{
    Path p(currentFileName());
    if (givenPath.isRelative() && p.valid())
    {
        givenPath = p.parent() / givenPath;
    }

    return givenPath;
}

//
// This function is used to assemble files referenced in the source code (via LOAD directive).
//
bool Assembler::assembleFile1(Path fileName)
{
    //
    // Step 1 - Find the file we're referring to
    //

    string fn = findFile(fileName).osPath();

    //
    // Step 2 - try to load it (if necessary)
    //

    if (m_sessions.find(fn) != m_sessions.end())
    {
        // We've seen this file before - no need for lexical analysis
        m_fileStack.emplace_back(fn);
    }
    else
    {
        const vector<u8> data = NxFile::loadFile(fn);
        if (!data.empty())
        {
            //
            // Lexical analysis
            //
            m_fileStack.emplace_back(fn);
            m_sessions[fn] = Lex();
            currentLex().parse(*this, std::move(data), fn);

#if NX_DEBUG_LOG_LEX
            dumpLex(m_sessions.back());
#endif // NX_DEBUG_LOG_LEX
        }
        else
        {
            output(stringFormat("!ERROR: Cannot open '{0}' for reading.", fn));
            return false;
        }
    }

    //
    // Pass 1
    //
    if (!pass1(currentLex(), currentLex().elements()))
    {
        m_fileStack.pop_back();
        return false;
    }

    m_fileStack.pop_back();
    return true;
}

bool Assembler::assembleFile2(Path fileName)
{
    //
    // Step 1 - Find the file we're referring to
    //

    string fn = findFile(fileName).osPath();

    //
    // Pass 2
    //
    m_fileStack.emplace_back(fn);
    bool result = pass2(currentLex(), currentLex().elements());
    m_fileStack.pop_back();
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
// Utility
//----------------------------------------------------------------------------------------------------------------------

void Assembler::output(const std::string& msg)
{
    m_assemblerWindow.output(msg);
}

void Assembler::addErrorInfo(const string& fileName, const string& message, int line, int col)
{
    m_errors.emplace_back(fileName, message, line, col);
}

void Assembler::error(const Lex& l, const Lex::Element& el, const string& message)
{
    Lex::Element::Pos start = el.m_position;
    int length = int(el.m_s1 - el.m_s0);

    addErrorInfo(l.getFileName(), message, start.m_line, start.m_col);

    // Output the error
    string err = stringFormat("!{0}({1}): {2}", l.getFileName(), start.m_line, message);
    output(err);

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

bool Assembler::addSymbol(i64 symbol, MemAddr address)
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
            m_values[symbol] = ExprValue(value);
            result = true;
        }
    }

    return result;
}

optional<MemAddr> Assembler::lookUpLabel(i64 symbol) const
{
    auto it = m_symbolTable.find(symbol);
    return it == m_symbolTable.end() ? optional<MemAddr>{} : it->second.m_addr;
}

optional<ExprValue> Assembler::lookUpValue(i64 symbol) const
{
    auto it = m_values.find(symbol);
    return it == m_values.end() ? optional<ExprValue>{} : it->second;
}


//----------------------------------------------------------------------------------------------------------------------
// Parsing utilities
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::expect(Lex& lex, const Lex::Element* e, const char* format, const Lex::Element** outE /* = nullptr */) const
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

        case 'x':
            pass = (e->m_type == T::IXH ||
                    e->m_type == T::IXL ||
                    e->m_type == T::IYH ||
                    e->m_type == T::IYL);
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
            pass = (e->m_type == T::IX || e->m_type == T::IY);
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

int Assembler::invalidInstruction(Lex& lex, const Lex::Element* e, const Lex::Element** outE)
{
    error(lex, *e, "Invalid instruction.");
    if (outE)
    {
        *outE = 0;
    }
    return 0;
}

bool Assembler::expectExpression(Lex& lex, const Lex::Element* e, const Lex::Element** outE) const
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

            case T::OpenParen:
                ++parenDepth;
                state = 0;
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

void Assembler::nextLine(const Lex::Element*& e)
{
    while (e->m_type != Lex::Element::Type::Newline) ++e;
    ++e;
}

//----------------------------------------------------------------------------------------------------------------------
// Pass 1
//----------------------------------------------------------------------------------------------------------------------

#define FAIL(msg)                               \
    do {                                        \
    error(lex, *e, (msg));                      \
    nextLine(e);                                \
    buildResult = false;                        \
    } while (0)

optional<MemAddr> Assembler::getZ80AddressFromExpression(Lex& lex, const Lex::Element* e, ExprValue expr)
{
    MemAddr a;
    bool buildResult = true;

    switch (expr.getType())
    {
    case ExprValue::Type::Integer:
        {
            if ((expr.getType() == ExprValue::Type::Integer) && ((i64)expr < 0x10000))
            {
                u16 addr = expr.r16();
                if (addr >= 0x4000 && addr <= 0xffff)
                {
                    a = m_speccy.convertAddress(Z80MemAddr((u16)addr));
                }
                else
                {
                    FAIL("Address out of range.  Must be between $4000-$ffff.");
                }
            }
            else
            {
                FAIL("Only addresses in 64K address space supported");
            }
        }
        break;

    case ExprValue::Type::Address:
        a = expr;
        if (!m_speccy.isZ80Address(a))
        {
            FAIL("Only addresses in 64K address space supported");
        }
        break;

    default:
        FAIL("Invalid expression found.");
    }

    return buildResult ? a : optional<MemAddr>{};
}

bool Assembler::pass1(Lex& lex, const vector<Lex::Element>& elems)
{
    output(lex.getFileName());

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

            if (outE)
            {
                e = outE;
            }
            else
            {
                buildResult = false;
                nextLine(e);
            }
        }
        else if (e->m_type > T::_END_OPCODES && e->m_type < T::_END_DIRECTIVES)
        {
            // It's a possible directive
            switch (e->m_type)
            {
            case T::ORG:
                {
                    const Lex::Element* endE;
                    if (expect(lex, ++e, "*", &endE))
                    {
                        Expression expr = buildExpression(e);
                        if (expr.eval(*this, lex, m_mmap.getAddress(m_address)))
                        {
                            optional<MemAddr> a = getZ80AddressFromExpression(lex, e, expr.result());

                            if (a)
                            {
                                // #todo: Support full address range
                                m_mmap.resetRange();
                                m_mmap.addZ80Range(m_speccy, m_speccy.convertAddress(*a), Z80MemAddr(0xffff));
                                m_address = 0;
                            }
                            else
                            {
                                buildResult = false;
                            }
                        }
                        else
                        {
                            buildResult = false;
                        }
                    }
                    else
                    {
                        FAIL("Invalid syntax for ORG directive.");
                    }
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

            case T::DB:
                ++e;
                while (e->m_type != T::Newline)
                {
                    const Lex::Element* outE;
                    if (expectExpression(lex, e, &outE))
                    {
                        // Expression found
                        ++m_address;
                        e = outE;
                    }
                    else if (e->m_type == T::String)
                    {
                        // String found
                        string str = (const char *)m_lexSymbols.get(e->m_symbol);
                        int strLen = int(m_lexSymbols.length(e->m_symbol));
                        m_address += strLen;
                        ++e;
                    }
                    else
                    {
                        FAIL("Invalid argument to a DB directive.");
                        break;
                    }

                    // Check for a comma
                    if (e->m_type == T::Comma)
                    {
                        if ((++e)->m_type == T::Newline)
                        {
                            FAIL("Invalid trailing comma.");
                            break;
                        }
                    }
                    else if (e->m_type != T::Newline)
                    {
                        FAIL("Comma expected.");
                        break;
                    }
                }
                ++e;
                symbolToAdd = true;     // We allow symbols on this line
                break;

            case T::DW:
                ++e;
                while (e->m_type != T::Newline)
                {
                    const Lex::Element* outE;
                    if (expectExpression(lex, e, &outE))
                    {
                        m_address += 2;
                        e = outE;
                    }
                    else
                    {
                        FAIL("Invalid argument to a DW directive.");
                        break;
                    }

                    // Check for comma
                    if (e->m_type == T::Comma)
                    {
                        if ((++e)->m_type == T::Newline)
                        {
                            FAIL("Invalid trailing comma.");
                            break;
                        }
                    }
                    else if (e->m_type != T::Newline)
                    {
                        FAIL("Comma expected.");
                        break;
                    }
                }
                ++e;
                symbolToAdd = true;
                break;

            case T::LOAD:
                ++e;
                if (e->m_type == T::String)
                {
                    string fileName = (const char *)m_lexSymbols.get(e->m_symbol);
                    if (!assembleFile1(fileName))
                    {
                        FAIL(stringFormat("Failed to assemble '{0}'.", fileName));
                    }
                    nextLine(e);
                }
                else
                {
                    FAIL("Invalid syntax for LOAD directive.  Expected a file name string.");
                }
                break;

            case T::OPT:
                nextLine(e);
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
#define CHECK_PARSE(n, format) PARSE(n, format); return invalidInstruction(lex, e, outE)

int Assembler::assembleInstruction1(Lex& lex, const Lex::Element* e, const Lex::Element** outE)
{
    using T = Lex::Element::Type;
    assert(e->m_type > T::_KEYWORDS && e->m_type < T::_END_OPCODES);

    switch (e->m_type)
    {
    case T::ADC:
        ++e;
        PARSE(1, "{abcdehl}");
        PARSE(1, "a,{abcdehl}");
        PARSE(2, "*");
        PARSE(2, "a,*");
        PARSE(1, "(H)");
        PARSE(1, "a,(H)");
        PARSE(2, "H,{BDHS}");
        PARSE(2, "a,x");
        PARSE(2, "x");
        CHECK_PARSE(3, "a,(%)");
        break;

    case T::ADD:
        ++e;
        PARSE(1, "{abcdehl}");
        PARSE(1, "a,{abcdehl}");
        PARSE(2, "*");
        PARSE(2, "a,*");
        PARSE(1, "(H)");
        PARSE(1, "a,(H)");
        PARSE(1, "H,{BDHS}");
        PARSE(2, "X,{BDXS}");
        PARSE(2, "Y,{BDYS}");
        PARSE(2, "a,x");
        PARSE(2, "x");
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
        PARSE(2, "a,x");
        PARSE(2, "x");
        CHECK_PARSE(3, "(%)");
        break;

    case T::CALL:
    case T::JP:
        ++e;
        PARSE(1, "(H)");
        PARSE(2, "({X})");
        PARSE(3, "*");
        CHECK_PARSE(3, "F,*");
        break;

    case T::CCF:
    case T::CPL:
    case T::DAA:
    case T::DI:
    case T::EI:
    case T::EXX:
    case T::HALT:
    case T::NOP:
    case T::RLA:
    case T::RRA:
    case T::RLCA:
    case T::RRCA:
    case T::SCF:
        ++e;
        CHECK_PARSE(1, "");

    case T::CPD:
    case T::CPDR:
    case T::CPI:
    case T::CPIR:
    case T::IND:
    case T::INDR:
    case T::INI:
    case T::INIR:
    case T::LDD:
    case T::LDDR:
    case T::LDI:
    case T::LDIR:
    case T::NEG:
    case T::OTDR:
    case T::OTIR:
    case T::OUTD:
    case T::OUTI:
    case T::RETI:
    case T::RETN:
    case T::RLD:
    case T::RRD:
        ++e;
        CHECK_PARSE(2, "");

    case T::DEC:
    case T::INC:
        ++e;
        PARSE(1, "{abcdehlBDHS}");
        PARSE(1, "(H)");
        PARSE(2, "{Xx}");
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
        CHECK_PARSE(2, "(S),{X}");
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
        CHECK_PARSE(2, "{X}");
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
    case T::SL1:
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
        PARSE(2, "a,x");
        PARSE(2, "x");
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
        PARSE(2, "a,x");
        PARSE(2, "x");
        CHECK_PARSE(3, "(%)");
        break;

    default:
        error(lex, *e, "Unimplemented instruction.");
        *outE = 0;
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
    PARSE(3, "{abcdehl},(%)");          // LD A/B/C/D/E/H/L,(IX/IY+nn)
    PARSE(3, "a,(*)");                  // LD A,(nnnn)
    PARSE(2, "{abcdehl},*");            // LD A/B/C/D/E/H/L,nn
    PARSE(3, "H,(*)");                  // LD HL,(nnnn)
    PARSE(4, "{BDXS},(*)");             // LD BC/DE/IX/IY/SP,(nnnn)
    PARSE(3, "{BDHS},*");               // LD BC/DE/HL/SP,nnnn
    PARSE(4, "{X},*");                  // LD IX/IY,nnnn
    PARSE(2, "(H),*");                  // LD (HL),nn
    PARSE(2, "a,i");                    // LD A,I
    PARSE(2, "i,a");                    // LD I,A
    PARSE(2, "a,r");                    // LD A,R
    PARSE(2, "r,a");                    // LD R,A
    PARSE(3, "a,(%)");                  // LD A,(IX+nn)
    PARSE(3, "(*),{aH}");               // LD (nnnn),A/HL
    PARSE(4, "(*),{BDXS}");             // LD (nnnn),BC/DE/IX/IY/SP
    PARSE(3, "(%),{abcdehl}");          // LD (IX/IY+nn),A/B/C/D/E/H/L
    PARSE(4, "(%),*");                  // LD (IX/IY+nn),nn
    PARSE(1, "S,H");                    // LD SP,HL
    PARSE(2, "S,{X}");                  // LD SP,IX/IY
    PARSE(2, "x,{abcdex}");             // LD IXH/IXL/IYH/IYL,A/B/C/D/E/IXH/IXL/IYH/IYL
    PARSE(2, "{abcdex},x");             // LD A/B/C/D/E/IXH/IXL/IYH/IYL,IXH/IXL/IYH/IYL
    PARSE(3, "x,*");                    // LD IXH/IXL/IYH/IYL,nn

    return invalidInstruction(lex, start, outE);
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

bool Assembler::Expression::eval(Assembler& assembler, Lex& lex, MemAddr currentAddress)
{
    // #todo: introduce types (value, address, page, offset etc) into expressions.

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
                    ((opInfo[(int)v.elem->m_type - (int)T::Plus].level > opInfo[(int)opStack.back().elem->m_type - (int)T::Plus].level))) &&
                opStack.back().type != ValueType::OpenParen)
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

    vector<ExprValue> stack;
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
                optional<MemAddr> value = assembler.lookUpLabel(v.value);
                if (value)
                {
                    stack.emplace_back(*value);
                }
                else
                {
                    optional<ExprValue> value = assembler.lookUpValue(v.value);
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
                if (stack.back().getType() == ExprValue::Type::Integer)
                {
                    stack.back() = ExprValue(-stack.back());
                }
                else
                {
                    FAIL();
                }
                break;

            case T::Tilde:
                if (stack.back().getType() == ExprValue::Type::Integer)
                {
                    stack.back() = ExprValue(~stack.back());
                }
                else
                {
                    FAIL();
                }
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
            
        default:
            assert(0);
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
    output(lex.getFileName());

    using T = Lex::Element::Type;
    const Lex::Element* e = elems.data();
    i64 symbol = 0;
    const Lex::Element* symE = 0;
    bool buildResult = true;
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
            int actualCount = m_address - oldAddress;
            assert(!outE || !count || (count == actualCount));
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

            case T::DB:
                buildResult = doDb(lex, ++e);
                break;

            case T::DW:
                buildResult = doDw(lex, ++e);
                break;

            case T::LOAD:
                {
                    ++e;
                    string fileName = string((char *)e->m_s0, (char *)e->m_s1);
                    buildResult = assembleFile2(fileName);
                }
                break;

            case T::OPT:
                buildResult = doOpt(lex, ++e);
                break;

            default:
                FAIL("Unimplemented directive.");
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


bool Assembler::CheckIntOpRange(Lex& lex, const Lex::Element* e, Operand op, i64 a, i64 b)
{
    ExprValue v = op.expr.result();

    if (v.getType() == ExprValue::Type::Integer)
    {
        if (v < a || v > b)
        {
            error(lex, *e, stringFormat("Integer expression out of range.  Must be be between {0} and {1}.", a, b));
            return false;
        }
    }
    else
    {
        error(lex, *e, "Invalid expression type.  Expecting an integer expression.");
        return false;
    }

    return true;
}

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
    const Lex::Element* dstE = nullptr;
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
        dstE = e;
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
            ++e;
        }
    }

    //
    // Step 4 - Assemble!
    //

#define UNDEFINED()                             \
    error(lex, *s, "Unimplemented opcode.");    \
    return nullptr;

#define CHECK8()                do { if (!CheckIntOpRange(lex, srcE, srcOp, 0, 255)) return nullptr; } while(0)
#define CHECK16()               do { if (!CheckIntOpRange(lex, srcE, srcOp, 0, 65535)) return nullptr; } while(0)
#define CHECK8_SIGNED()         do { if (!CheckIntOpRange(lex, srcE, srcOp, -128, 127)) return nullptr; } while(0)
#define CHECK16_SIGNED()        do { if (!CheckIntOpRange(lex, srcE, srcOp, -32768, 32767)) return nullptr; } while(0)

#define CHECK8_DST()            do { if (!CheckIntOpRange(lex, dstE, dstOp, 0, 255)) return nullptr; } while(0)
#define CHECK16_DST()           do { if (!CheckIntOpRange(lex, dstE, dstOp, 0, 65535)) return nullptr; } while(0)
#define CHECK8_DST_SIGNED()     do { if (!CheckIntOpRange(lex, dstE, dstOp, -128, 127)) return nullptr; } while(0)
#define CHECK16_DST_SIGNED()    do { if (!CheckIntOpRange(lex, dstE, dstOp, -32768, 32767)) return nullptr; } while(0)

    // These values are used to generate the machine code for the instruction
    u8 indexPrefix = 0;
    u8 prefix = 0;
    u8 indexOffset = 0;
    bool addressIndex = false;
    u8 x = 0;
    u8 p = 0;
    u8 q = 0;
    u8 z = 0;
    u8 op8 = 0;
    u16 op16 = 0;
    int opSize = 0;

#define XPQZ(xx, pp, qq, zz) do {                                       \
        x = (xx);                                                       \
        p = (pp);                                                       \
        q = (qq);                                                       \
        z = (zz);                                                       \
    } while(0)

#define XYZ(xx, yy, zz) do {                                            \
        x = (xx);                                                       \
        p = (yy) >> 1;                                                  \
        q = (yy) & 1;                                                   \
        z = (zz);                                                       \
    } while(0)

#define SRC_OP8() do {                                                  \
        opSize = 1;                                                     \
        op8 = (srcOp.expr.r8());                                        \
    } while(0)

#define SRC_OP16() do {                                                 \
        opSize = 2;                                                     \
        op16 = (srcOp.expr.r16());                                      \
    } while(0)

#define DST_OP8() do {                                                  \
        opSize = 1;                                                     \
        op8 = (dstOp.expr.r8());                                        \
    } while(0)

#define DST_OP16() do {                                                 \
        opSize = 2;                                                     \
        op16 = (dstOp.expr.r16());                                      \
    } while(0)

    //
    // Check for IX/IY
    //

    switch (dstOp.type)
    {
    case OperandType::IX_Expression:
        CHECK8_DST_SIGNED();
        indexPrefix = 0xdd;
        dstOp.type = OperandType::Address_HL;
        indexOffset = dstOp.expr.r8();
        addressIndex = true;
        break;

    case OperandType::IX:
        indexPrefix = 0xdd;
        dstOp.type = OperandType::HL;
        break;

    case OperandType::IY_Expression:
        CHECK8_DST_SIGNED();
        indexPrefix = 0xfd;
        dstOp.type = OperandType::Address_HL;
        indexOffset = dstOp.expr.r8();
        addressIndex = true;
        break;

    case OperandType::IY:
        indexPrefix = 0xfd;
        dstOp.type = OperandType::HL;
        break;

    case OperandType::IXH:
        indexPrefix = 0xdd;
        dstOp.type = OperandType::H;
        break;

    case OperandType::IXL:
        indexPrefix = 0xdd;
        dstOp.type = OperandType::L;
        break;

    case OperandType::IYH:
        indexPrefix = 0xfd;
        dstOp.type = OperandType::H;
        break;

    case OperandType::IYL:
        indexPrefix = 0xfd;
        dstOp.type = OperandType::L;
        break;

    default:
        break;
    }

    // Check we don't have a mix:
    switch (srcOp.type)
    {
    case OperandType::IX_Expression:
    case OperandType::IX:
    case OperandType::IXH:
    case OperandType::IXL:
        if (indexPrefix == 0xfd)
        {
            error(lex, *srcE, "Cannot have both IX and IY registers in same instruction.");
            return 0;
        }
        break;

    case OperandType::IY_Expression:
    case OperandType::IY:
    case OperandType::IYH:
    case OperandType::IYL:
        if (indexPrefix == 0xdd)
        {
            error(lex, *srcE, "Cannot have both IX and IY registers in same instruction.");
            return 0;
        }
        break;

    default:
        break;
    }

    switch (srcOp.type)
    {
    case OperandType::IX_Expression:
        CHECK8_SIGNED();
        indexPrefix = 0xdd;
        srcOp.type = OperandType::Address_HL;
        assert(!addressIndex);
        indexOffset = srcOp.expr.r8();
        addressIndex = true;
        break;

    case OperandType::IX:
        indexPrefix = 0xdd;
        srcOp.type = OperandType::HL;
        break;

    case OperandType::IY_Expression:
        CHECK8_SIGNED();
        indexPrefix = 0xfd;
        srcOp.type = OperandType::Address_HL;
        assert(!addressIndex);
        indexOffset = srcOp.expr.r8();
        addressIndex = true;
        break;

    case OperandType::IY:
        indexPrefix = 0xfd;
        srcOp.type = OperandType::HL;
        break;

    case OperandType::IXH:
        indexPrefix = 0xdd;
        srcOp.type = OperandType::H;
        break;

    case OperandType::IXL:
        indexPrefix = 0xdd;
        srcOp.type = OperandType::L;
        break;

    case OperandType::IYH:
        indexPrefix = 0xfd;
        srcOp.type = OperandType::H;
        break;

    case OperandType::IYL:
        indexPrefix = 0xfd;
        srcOp.type = OperandType::L;
        break;

    default:
        break;
    }

    switch (opCode)
    {
        //--------------------------------------------------------------------------------------------------------------
        // ALU

    case T::ADD:
        switch (dstOp.type)
        {
        case OperandType::HL:
            switch (srcOp.type)
            {
            case OperandType::BC:
            case OperandType::DE:
            case OperandType::HL:
            case OperandType::SP:
                XPQZ(0, rp(srcOp.type), 1, 1);
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::A:
            // ADD A,... or ADD A
            switch (srcOp.type)
            {
            case OperandType::None:
                XYZ(2, 0, 7);
                break;

            case OperandType::B:
            case OperandType::C:
            case OperandType::D:
            case OperandType::E:
            case OperandType::H:
            case OperandType::L:
            case OperandType::Address_HL:
            case OperandType::A:
                XYZ(2, 0, r(srcOp.type));
                break;

            case OperandType::Expression:
                CHECK8();
                XYZ(3, 0, 6);
                SRC_OP8();
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::B:
        case OperandType::C:
        case OperandType::D:
        case OperandType::E:
        case OperandType::H:
        case OperandType::L:
        case OperandType::Address_HL:
            assert(srcOp.type == OperandType::None);
            XYZ(2, 0, r(dstOp.type));
            break;

        default: UNDEFINED();
        }
        break;

    case T::ADC:
    case T::SBC:
    case T::SUB:
    case T::AND:
    case T::XOR:
    case T::OR:
    case T::CP:
        switch (dstOp.type)
        {
        case OperandType::A:
            // <ALU> A,... or <ALU> A
            switch (srcOp.type)
            {
            case OperandType::None:                                     // <ALU> A
                XYZ(2, alu(opCode), 7);
                break;

            case OperandType::Expression:                               // <ALU> A,nn
                CHECK8();
                XYZ(3, alu(opCode), 6);
                SRC_OP8();
                break;

            case OperandType::A:                                        // <ALU> A,r
            case OperandType::B:
            case OperandType::C:
            case OperandType::D:
            case OperandType::E:
            case OperandType::H:
            case OperandType::L:
            case OperandType::Address_HL:
                XYZ(2, alu(opCode), r(srcOp.type));
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::B:
        case OperandType::C:
        case OperandType::D:
        case OperandType::E:
        case OperandType::H:
        case OperandType::L:
        case OperandType::Address_HL:
            assert(srcOp.type == OperandType::None);
            XYZ(2, alu(opCode), r(dstOp.type));
            break;

        case OperandType::Expression:
            CHECK8_DST();
            XYZ(3, alu(opCode), 6);
            DST_OP8();
            break;

        case OperandType::HL:
            assert(opCode == T::SBC || opCode == T::ADC);
            prefix = 0xed;
            XPQZ(1, rp(srcOp.type), opCode == T::ADC ? 1 : 0, 2);
            break;

        default: UNDEFINED();
        }
        break;

        //--------------------------------------------------------------------------------------------------------------
        // BIT OPERATIONS

    case T::BIT:
    case T::RES:
    case T::SET:
        {
            if (dstOp.expr.result() < 0 || dstOp.expr.result() > 7)
            {
                error(lex, *dstE, "Invalid bit index.  Must be 0-7.");
                return nullptr;
            }
            prefix = 0xcb;
            u8 xx = (opCode == T::BIT ? 1 : opCode == T::RES ? 2 : 3);
            XYZ(xx, dstOp.expr.r8(), r(srcOp.type));
        }
        break;
        
        //--------------------------------------------------------------------------------------------------------------
        // ROTATION AND SHIFTING

    case T::RLC:
    case T::RRC:
    case T::RL:
    case T::RR:
    case T::SLA:
    case T::SRA:
    case T::SLL:
    case T::SL1:
    case T::SRL:
        prefix = 0xcb;
        XYZ(0, rot(opCode), r(dstOp.type));
        break;

        //--------------------------------------------------------------------------------------------------------------
        // BRANCHES

    case T::CALL:
        switch (dstOp.type)
        {
        case OperandType::NZ:               // JP cc,NNNN
        case OperandType::Z:
        case OperandType::NC:
        case OperandType::C:
        case OperandType::PO:
        case OperandType::PE:
        case OperandType::P:
        case OperandType::M:
            CHECK16();
            XYZ(3, cc(dstOp.type), 4);
            SRC_OP16();
            break;

        case OperandType::Expression:
            CHECK16_DST();
            XPQZ(3, 0, 1, 5);
            DST_OP16();
            break;

        default: UNDEFINED();
        }
        break;

    case T::DJNZ:
        if (optional<u8> d = calculateDisplacement(lex, dstE, dstOp.expr); d)
        {
            XYZ(0, 2, 0);
            op8 = *d;
            opSize = 1;
        }
        break;

    case T::JP:
        switch (dstOp.type)
        {
        case OperandType::HL:               // JP HL
        case OperandType::Address_HL:       // JP (HL)
            if (addressIndex)
            {
                // We need to make sure that the offset is only 0, because JP (IX) is allowed,
                // but JP (IX+n) is not.
                if (indexOffset != 0)
                {
                    error(lex, *dstE, "Index offsets are not allowed in JP instructions.  Remove the offset.");
                    return nullptr;
                }

                // We don't want to emit the offset later.
                addressIndex = false;
            }
            XPQZ(3, 2, 1, 1);
            break;

        case OperandType::NZ:               // JP cc,NNNN
        case OperandType::Z:
        case OperandType::NC:
        case OperandType::C:
        case OperandType::PO:
        case OperandType::PE:
        case OperandType::P:
        case OperandType::M:
            CHECK16();
            XYZ(3, cc(dstOp.type),2);
            SRC_OP16();
            break;

        case OperandType::Expression:
            CHECK16_DST();
            XYZ(3, 0, 3);
            DST_OP16();
            break;

        default: UNDEFINED();
        }
        break;

    case T::JR:
        switch (dstOp.type)
        {
        case OperandType::Expression:
            // JR d
            if (optional<u8> d = calculateDisplacement(lex, dstE, dstOp.expr); d)
            {
                XYZ(0, 3, 0);
                op8 = *d;
                opSize = 1;
            }
            break;

        case OperandType::NZ:
        case OperandType::Z:
        case OperandType::NC:
        case OperandType::C:
            // JR cc,d
            if (optional<u8> d = calculateDisplacement(lex, srcE, srcOp.expr); d)
            {
                XYZ(0, cc(dstOp.type) + 4, 0);
                op8 = *d;
                opSize = 1;
            }
            break;

        default: UNDEFINED();
        }
        break;

    case T::RET:
        if (dstOp.type == OperandType::None)
        {
            XPQZ(3, 0, 1, 1);
        }
        else
        {
            XYZ(3, cc(dstOp.type), 0);
        }
        break;

    case T::RST:
        if (dstOp.expr.result() < 0 ||
            dstOp.expr.result() > 0x56 ||
            ((i64)dstOp.expr.result() % 8) != 0)
        {
            error(lex, *dstE, "Invalid value for RST opcode.");
            return nullptr;
        }
        XYZ(3, u8((i64)dstOp.expr.result() / 8), 7);
        break;

        //--------------------------------------------------------------------------------------------------------------
        // INC/DEC

    case T::DEC:
        switch (dstOp.type)
        {
        case OperandType::BC:
        case OperandType::DE:
        case OperandType::HL:
        case OperandType::SP:
            XPQZ(0, rp(dstOp.type), 1, 3);
            break;

        case OperandType::B:
        case OperandType::C:
        case OperandType::D:
        case OperandType::E:
        case OperandType::H:
        case OperandType::L:
        case OperandType::Address_HL:
        case OperandType::A:
            XYZ(0, r(dstOp.type), 5);
            break;

        default: UNDEFINED();
        }
        break;

    case T::INC:
        switch (dstOp.type)
        {
        case OperandType::BC:
        case OperandType::DE:
        case OperandType::HL:
        case OperandType::SP:
            XPQZ(0, rp(dstOp.type), 0, 3);
            break;

        case OperandType::B:
        case OperandType::C:
        case OperandType::D:
        case OperandType::E:
        case OperandType::H:
        case OperandType::L:
        case OperandType::Address_HL:
        case OperandType::A:
            XYZ(0, r(dstOp.type), 4);
            break;

        default: UNDEFINED();
        }
        break;

        //--------------------------------------------------------------------------------------------------------------
        // LD

    case T::LD:
        switch (dstOp.type)
        {
        case OperandType::A:
            switch (srcOp.type)
            {
            case OperandType::Expression:                       // LD A,nn
                CHECK8();
                XYZ(0, 7, 6);
                SRC_OP8();
                break;

            case OperandType::Address_BC:                       // LD A,(BC)
            case OperandType::Address_DE:
                XPQZ(0, rp(srcOp.type), 1, 2);
                break;

            case OperandType::AddressedExpression:              // LD A,(nnnn)
                CHECK16();
                XPQZ(0, 3, 1, 2);
                SRC_OP16();
                break;

            case OperandType::B:
            case OperandType::C:
            case OperandType::D:
            case OperandType::E:
            case OperandType::H:
            case OperandType::L:
            case OperandType::Address_HL:
            case OperandType::A:
                XYZ(1, 7, r(srcOp.type));
                break;

            case OperandType::I:                                // LD A,I
                prefix = 0xed;
                XYZ(1, 2, 7);
                break;

            case OperandType::R:                                // LD A,R
                prefix = 0xed;
                XYZ(1, 3, 7);
                break;

            default: UNDEFINED();
            }
            break;

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
                XYZ(0, r(dstOp.type), 6);
                SRC_OP8();
                break;

            case OperandType::B:
            case OperandType::C:
            case OperandType::D:
            case OperandType::E:
            case OperandType::H:
            case OperandType::L:
            case OperandType::Address_HL:
            case OperandType::A:
                // This is to check for LD (HL),(HL) combination, which doesn't exist.
                assert(dstOp.type != OperandType::Address_HL || srcOp.type != OperandType::Address_HL);
                XYZ(1, r(dstOp.type), r(srcOp.type));
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::BC:
        case OperandType::DE:
            switch (srcOp.type)
            {
            case OperandType::Expression:
                CHECK16();
                XPQZ(0, rp(dstOp.type), 0, 1);
                SRC_OP16();
                break;

            case OperandType::AddressedExpression:
                CHECK16();
                prefix = 0xed;
                XPQZ(1, rp(dstOp.type), 1, 3);
                SRC_OP16();
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::SP:
            switch (srcOp.type)
            {
            case OperandType::Expression:
                CHECK16();
                XPQZ(0, 3, 0, 1);
                SRC_OP16();
                break;

            case OperandType::HL:
                XPQZ(3, 3, 1, 1);
                break;

            case OperandType::AddressedExpression:
                CHECK16();
                prefix = 0xed;
                XPQZ(1, 3, 1, 3);
                SRC_OP16();
                break;

            default: UNDEFINED();
            }
            break;


        case OperandType::HL:
            switch (srcOp.type)
            {
            case OperandType::Expression:               // LD HL,nnnn
                CHECK16();
                XPQZ(0, 2, 0, 1);
                SRC_OP16();
                break;

            case OperandType::AddressedExpression:      // LD HL,(nnnn)
                CHECK16();
                XPQZ(0, 2, 1, 2);
                SRC_OP16();
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::Address_BC:
        case OperandType::Address_DE:
            if (srcOp.type == OperandType::A)           // LD (BC/DE),A
            {
                XPQZ(0, rp(dstOp.type), 0, 2);
            }
            else
            {
                assert(srcOp.type == OperandType::AddressedExpression);
                CHECK16();
                XPQZ(1, rp(dstOp.type), 1, 3);
                SRC_OP16();
            }
            break;

        case OperandType::AddressedExpression:
            switch (srcOp.type)
            {
            case OperandType::HL:                       // LD (nnnn),HL
                CHECK16_DST();
                XPQZ(0, 2, 0, 2);
                DST_OP16();
                break;

            case OperandType::A:                        // LD (nnnn),A
                CHECK16_DST();
                XPQZ(0, 3, 0, 2);
                DST_OP16();
                break;

            case OperandType::BC:                       // LD (nnnn),rr
            case OperandType::DE:
            case OperandType::SP:
                CHECK16_DST();
                prefix = 0xed;
                XPQZ(1, rp(srcOp.type), 0, 3);
                DST_OP16();
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::I:                            // LD I,A
            assert(srcOp.type == OperandType::A);
            prefix = 0xed;
            XYZ(1, 0, 7);
            break;

        case OperandType::R:                            // LD R,A
            assert(srcOp.type == OperandType::A);
            prefix = 0xed;
            XYZ(1, 1, 7);
            break;

        default: UNDEFINED();
        } // dstOp.type
        break;

        //--------------------------------------------------------------------------------------------------------------
        // IN/OUT

    case T::IN:
        switch (dstOp.type)
        {
        case OperandType::A:
            if (srcOp.type == OperandType::AddressedExpression)     // IN A,(nn)
            {
                CHECK8();
                XYZ(3, 3, 3);
                SRC_OP8();
                break;
            }
            // continue
        case OperandType::B:
        case OperandType::C:
        case OperandType::D:
        case OperandType::E:
        case OperandType::H:
        case OperandType::L:
            assert(srcOp.type == OperandType::Address_C);
            prefix = 0xed;
            XYZ(1, r(dstOp.type), 0);
            break;

        case OperandType::None:
            assert(srcOp.type == OperandType::Address_C);
            prefix = 0xed;
            XYZ(1, 6, 0);
            break;

        default: UNDEFINED();
        }
        break;

    case T::OUT:
        switch (dstOp.type)
        {
        case OperandType::AddressedExpression:
            switch (srcOp.type)
            {
            case OperandType::A:
                CHECK8_DST();
                XYZ(3, 2, 3);
                DST_OP8();
                break;

            default: UNDEFINED();
            }
            break;

        case OperandType::Address_C:
            switch (srcOp.type)
            {
            case OperandType::B:
            case OperandType::C:
            case OperandType::D:
            case OperandType::E:
            case OperandType::H:
            case OperandType::L:
            case OperandType::A:
                prefix = 0xed;
                XYZ(1, r(srcOp.type), 1);
                break;

            case OperandType::Expression:
                if (srcOp.expr.result() != 0)
                {
                    error(lex, *srcE, "Invalid expression for OUT instruction.  Must be 0 or 8-bit register.");
                    return nullptr;
                }
                prefix = 0xed;
                XYZ(1, 6, 1);
                break;

            default: UNDEFINED();
            }
            break;

        default: UNDEFINED();
        }
        break;

        //--------------------------------------------------------------------------------------------------------------
        // Other opcodes

    case T::EX:
        switch (dstOp.type)
        {
        case OperandType::AF:
            XYZ(0, 1, 0);
            break;

        case OperandType::Address_SP:
            assert(srcOp.type == OperandType::HL);
            XYZ(3, 4, 3);
            break;

        case OperandType::DE:
            assert(srcOp.type == OperandType::HL);
            XYZ(3, 5, 3);
            break;

        default: UNDEFINED();
        }
        break;

    case T::IM:
        if (dstOp.expr.result() < 0 || dstOp.expr.result() > 2)
        {
            error(lex, *dstE, "Invalid value of IM instruction.  Must be 0-2.");
            return nullptr;
        }
        prefix = 0xed;
        switch (dstOp.expr.result())
        {
        case 0: XYZ(1, 0, 6);   break;
        case 1: XYZ(1, 2, 6);   break;
        case 2: XYZ(1, 3, 6);   break;
        }
        break;

    case T::POP:
        XPQZ(3, rp2(dstOp.type), 0, 1);
        break;

    case T::PUSH:
        XPQZ(3, rp2(dstOp.type), 0, 5);
        break;

        //--------------------------------------------------------------------------------------------------------------
        // Single opcodes

    case T::CCF:    XYZ(0, 7, 7);       break;
    case T::CPL:    XYZ(0, 5, 7);       break;
    case T::DAA:    XYZ(0, 4, 7);       break;
    case T::DI:     XYZ(3, 6, 3);       break;
    case T::EI:     XYZ(3, 7, 3);       break;
    case T::EXX:    XPQZ(3, 1, 1, 1);   break;
    case T::HALT:   XYZ(1, 6, 6);       break;
    case T::NOP:    XYZ(0, 0, 0);       break;
    case T::RLA:    XYZ(0, 2, 7);       break;
    case T::RLCA:   XYZ(0, 0, 7);       break;
    case T::RRA:    XYZ(0, 3, 7);       break;
    case T::RRCA:   XYZ(0, 1, 7);       break;
    case T::SCF:    XYZ(0, 6, 7);       break;

    case T::NEG:    prefix = 0xed;      XYZ(1, 0, 4);       break;
    case T::RETN:   prefix = 0xed;      XYZ(1, 0, 5);       break;
    case T::RETI:   prefix = 0xed;      XYZ(1, 1, 5);       break;
    case T::RLD:    prefix = 0xed;      XYZ(1, 5, 7);       break;
    case T::RRD:    prefix = 0xed;      XYZ(1, 4, 7);       break;

    case T::LDI:    prefix = 0xed;      XYZ(2, 4, 0);       break;
    case T::LDD:    prefix = 0xed;      XYZ(2, 5, 0);       break;
    case T::LDIR:   prefix = 0xed;      XYZ(2, 6, 0);       break;
    case T::LDDR:   prefix = 0xed;      XYZ(2, 7, 0);       break;

    case T::CPI:    prefix = 0xed;      XYZ(2, 4, 1);       break;
    case T::CPD:    prefix = 0xed;      XYZ(2, 5, 1);       break;
    case T::CPIR:   prefix = 0xed;      XYZ(2, 6, 1);       break;
    case T::CPDR:   prefix = 0xed;      XYZ(2, 7, 1);       break;

    case T::INI:    prefix = 0xed;      XYZ(2, 4, 2);       break;
    case T::IND:    prefix = 0xed;      XYZ(2, 5, 2);       break;
    case T::INIR:   prefix = 0xed;      XYZ(2, 6, 2);       break;
    case T::INDR:   prefix = 0xed;      XYZ(2, 7, 2);       break;

    case T::OUTI:   prefix = 0xed;      XYZ(2, 4, 3);       break;
    case T::OUTD:   prefix = 0xed;      XYZ(2, 5, 3);       break;
    case T::OTIR:   prefix = 0xed;      XYZ(2, 6, 3);       break;
    case T::OTDR:   prefix = 0xed;      XYZ(2, 7, 3);       break;

    default: UNDEFINED();
    }

#undef UNDEFINED
#undef CHECK8
#undef CHECK16
#undef CHECK8_SIGNED
#undef CHECK16_SIGNED
#undef CHECK8_DST
#undef CHECK16_DST
#undef CHECK8_DST_SIGNED
#undef CHECK16_DST_SIGNED

#undef XPQZ
#undef XYZ
#undef SRC_OP8
#undef SRC_OP16
#undef DST_OP8
#undef DST_OP16

    //
    // Step 5 - Figure out the machine code
    //
    // Possibilities
    //                                                              Indexed     Offset  Prefixed    opSize
    //  O           Single opcodes                                    NO          NO      NO          0
    //  ON          8-bit operands with an opcode                     NO          NO      NO          1
    //  ONN         16-bit operand with an opcode                     NO          NO      NO          2
    //  PO          Prefixed opcode (0xCB, 0xED)                      NO          NO      YES         0
    //  PONN        16-bit operand with prefixed opcode               NO          NO      YES         2
    //  XO          Indexed opcode (0xDD, 0xFD)                       YES         NO      NO          0
    //  XOF         Address Indexed opcode (0xDD, 0xFD)               YES         YES     NO          0
    //  XON         8-bit operands with indexed opcode                YES         NO      NO          1
    //  XOFN        8-bit operands with addressed indexed opcode      YES         YES     NO          1
    //  XONN        16-bit operands with indexed opcode               YES         NO      NO          2
    //  XPNO        8-bit operands with prefixed indexed opcode       YES         NO      YES         1
    //
    //  O = opcode, N = 8-bit/16-bit value, F = Index offset, P = prefix

    if (indexPrefix)
    {
        // Has IX or IY in instruction
        emit8(indexPrefix);
        if (prefix)
        {
            // Has $CB has prefix
            emit8(prefix);
            emit8(indexOffset);
            emitXPQZ(x, p, q, z);
        }
        else
        {
            emitXPQZ(x, p, q, z);

            if (addressIndex)
            {
                assert(opSize < 2);
                emit8(indexOffset);
            }

            switch (opSize)
            {
            case 0:                 break;
            case 1: emit8(op8);     break;
            case 2: emit16(op16);   break;
            default: assert(0);
            }
        }
    }
    else
    {
        if (prefix) emit8(prefix);
        emitXPQZ(x, p, q, z);
        switch (opSize)
        {
        case 0:                                    break;
        case 1: assert(!prefix);    emit8(op8);    break;
        case 2:                     emit16(op16);  break;
        default: assert(0);
        }
    }

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
        switch ((e++)->m_type)
        {
        case T::C:
            op.type = OperandType::Address_C;
            assert(e->m_type == T::CloseParen);
            break;

        case T::BC:
            op.type = OperandType::Address_BC;
            assert(e->m_type == T::CloseParen);
            break;

        case T::DE:
            op.type = OperandType::Address_DE;
            assert(e->m_type == T::CloseParen);
            break;

        case T::HL:
            op.type = OperandType::Address_HL;
            assert(e->m_type == T::CloseParen);
            break;

        case T::SP:
            op.type = OperandType::Address_SP;
            assert(e->m_type == T::CloseParen);
            break;

        case T::IX:
            if (e->m_type == T::CloseParen)
            {
                // This is just (IX), no expression
                op.expr.set(0);
            }
            else
            {
                op.expr = buildExpression(e);
                if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
            }
            op.type = OperandType::IX_Expression;
            assert(e->m_type == T::CloseParen);
            break;

        case T::IY:
            if (e->m_type == T::CloseParen)
            {
                // This is just (IY), no expression
                op.expr.set(0);
            }
            else
            {
                op.expr = buildExpression(e);
                if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
            }
            op.type = OperandType::IY_Expression;
            assert(e->m_type == T::CloseParen);
            break;

        default:
            // Must be an address expression
            {
                const Lex::Element* startE = e - 2;
                op.type = OperandType::AddressedExpression;
                op.expr = buildExpression(--e);
                assert(e->m_type == T::CloseParen);
                Lex::Element::Type nextE = (e + 1)->m_type;
                if (nextE == T::Newline || nextE == T::Comma)
                {
                    if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
                }
                else
                {
                    // This is an expression, not an addressed expression because there are extraneous
                    // tokens after the close parentheses matching the initial opening one.
                    // Rebuild the expression, this time including the initial parentheses.
                    op.type = OperandType::Expression;
                    e = startE;
                    op.expr = buildExpression(e);
                    if (!op.expr.eval(*this, lex, m_mmap.getAddress(m_address))) return 0;
                    --e;
                }
            }
            break;
        }

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
    case T::IXH:    op.type = OperandType::IXH;         ++e; break;
    case T::IXL:    op.type = OperandType::IXL;         ++e; break;
    case T::IYH:    op.type = OperandType::IYH;         ++e; break;
    case T::IYL:    op.type = OperandType::IYL;         ++e; break;
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

Assembler::Expression Assembler::buildExpression(const Lex::Element*& e) const
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
                assert(parenDepth == 0);
                return expr;

            case T::Newline:
                assert(parenDepth == 0);
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
            case T::OpenParen:
                expr.addOpen(e);
                ++parenDepth;
                state = 0;
                break;
            default:
                assert(0);
            }
        }

        ++e;
    }
}

optional<u8> Assembler::calculateDisplacement(Lex& lex, const Lex::Element* e, Expression& expr)
{
    // #todo: Handle pages
    MemAddr a0 = m_mmap.getAddress(m_address) + 2;                                  // Current address
    optional<MemAddr> a1 = getZ80AddressFromExpression(lex, e, expr.result());      // Address we want to go to
    if (a1)
    {
        int d = *a1 - a0;
        if (d < -128 || d > 127)
        {
            error(lex, *e, stringFormat("Relative jump of {0} is too far.  Distance must be between -128 and +127.", d));
            return {};
        }

        return u8(d);
    }
    else
    {
        error(lex, *e, "Invalid expression for displacement value.");
    }

    return {};
}

optional<i64> Assembler::calculateExpression(const vector<u8>& exprData)
{
    //
    // Lexical analysis on the expression
    //
    Lex lex;
    if (!lex.parse(*this, exprData, "<input>")) return {};

    //
    // Check the syntax
    //
    const Lex::Element* start = lex.elements().data();
    const Lex::Element* end;
    if (!expect(lex, lex.elements().data(), "*", &end)) return {};
    if (end->m_type != Lex::Element::Type::Newline) return {};

    //
    // Calculate
    //
    Expression expr = buildExpression(start);
    if (!expr.eval(*this, lex, MemAddr())) return {};

    return expr.result();
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
    case OperandType::Address_BC:
    case OperandType::BC:       return 0;
    case OperandType::Address_DE:
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

u8 Assembler::cc(OperandType ot) const
{
    switch (ot)
    {
    case OperandType::NZ:       return 0;
    case OperandType::Z:        return 1;
    case OperandType::NC:       return 2;
    case OperandType::C:        return 3;
    case OperandType::PO:       return 4;
    case OperandType::PE:       return 5;
    case OperandType::P:        return 6;
    case OperandType::M:        return 7;

    default:
        assert(0);
    }

    return 0;
}

u8 Assembler::rot(Lex::Element::Type opCode) const
{
    switch (opCode)
    {
    case Lex::Element::Type::RLC:   return 0;
    case Lex::Element::Type::RRC:   return 1;
    case Lex::Element::Type::RL:    return 2;
    case Lex::Element::Type::RR:    return 3;
    case Lex::Element::Type::SLA:   return 4;
    case Lex::Element::Type::SRA:   return 5;
    case Lex::Element::Type::SL1:
    case Lex::Element::Type::SLL:   return 6;
    case Lex::Element::Type::SRL:   return 7;

    default:
        assert(0);
    }

    return 0;
}

u8 Assembler::alu(Lex::Element::Type opCode) const
{
    switch (opCode)
    {
    case Lex::Element::Type::ADD:   return 0;
    case Lex::Element::Type::ADC:   return 1;
    case Lex::Element::Type::SUB:   return 2;
    case Lex::Element::Type::SBC:   return 3;
    case Lex::Element::Type::AND:   return 4;
    case Lex::Element::Type::XOR:   return 5;
    case Lex::Element::Type::OR:    return 6;
    case Lex::Element::Type::CP:    return 7;

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
    m_mmap.poke16(m_address, w);
    m_address += 2;
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
    if (!addr.eval(*this, lex, m_mmap.getAddress(m_address))) return false;
    if (addr.result() < 0 || addr.result() > 0xffff)
    {
        error(lex, *e, "Address out of range.");
        while (e->m_type != T::Newline) ++e;
        ++e;
        return false;
    }
    
    u16 p = u16(addr.result());
    m_mmap.resetRange();
    m_mmap.addZ80Range(m_speccy, p, 0xffff);
    m_address = 0;
    return true;
}

bool Assembler::doEqu(Lex& lex, i64 symbol, const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    Expression expr = buildExpression(e);
    if (!expr.eval(*this, lex, m_mmap.getAddress(m_address))) return false;

    if (!addValue(symbol, expr.result()))
    {
        error(lex, *e, "Variable name already used.");
        while (e->m_type != T::Newline) ++e;
        ++e;
        return false;
    }

    return true;
}

bool Assembler::doDb(Lex& lex, const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    while (e->m_type != T::Newline)
    {
        const Lex::Element* outE = nullptr;
        if (expectExpression(lex, e, &outE))
        {
            // Expression found
            const Lex::Element* startE = e;
            Expression expr = buildExpression(e);
            if (!expr.eval(*this, lex, m_mmap.getAddress(m_address))) return false;
            if (expr.result() < -128 || expr.result() > 255)
            {
                error(lex, *startE, "Byte value is out of range.  Must be -128 to +127 or 0-255.");
                while (e->m_type != T::Newline) ++e;
                ++e;
                return false;
            }

            emit8(expr.r8());
        }
        else if (e->m_type == T::String)
        {
            const char* str = (const char *)m_lexSymbols.get(e->m_symbol);
            const char* end = str + m_lexSymbols.length(e->m_symbol);
            for (; str != end; ++str)
            {
                emit8(*str);
            }
            ++e;
        }

        if (e->m_type == T::Comma) ++e;
    }

    return true;
}

bool Assembler::doDw(Lex& lex, const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    while (e->m_type != T::Newline)
    {
        const Lex::Element* outE = nullptr;
        if (expectExpression(lex, e, &outE))
        {
            // Expression found
            const Lex::Element* startE = e;
            Expression expr = buildExpression(e);
            if (!expr.eval(*this, lex, m_mmap.getAddress(m_address))) return false;
            if (expr.result() < -32768 || expr.result() > 65535)
            {
                error(lex, *startE, "Word value is out of range.  Must be -32768 to 65535.");
                nextLine(e);
                return false;
            }

            emit16(expr.r16());
        }

        if (e->m_type == T::Comma) ++e;
    }

    return true;
}

bool Assembler::doOpt(Lex& lex, const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    i64 startSym = m_lexSymbols.addString("start", true);

    i64 option = 0;
    vector<const Lex::Element*> args;

    if (e->m_type == T::Symbol)
    {
        option = e->m_symbol;
        ++e;

        if (e->m_type == T::Colon)
        {
            ++e;
        }
        else if (e->m_type != T::Newline)
        {
            error(lex, *e, "Invalid option syntax.");
            nextLine(e);
            return false;
        }

        if (option == startSym)         return doOptStart(lex, e);
        else
        {
            error(lex, *e, "Unknown option.");
            nextLine(e);
            return false;
        }
    }
    else
    {
        error(lex, *e, "Invalid option syntax.");
        nextLine(e);
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Options
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::doOptStart(Lex& lex, const Lex::Element*& e)
{
    if (!expect(lex, e, "*"))
    {
        error(lex, *e, "Syntax error in START option.  Should be \"START:<address>\".");
        nextLine(e);
        return false;
    }

    Expression expr = buildExpression(e);
    // #todo: Handle full addresses
    optional<MemAddr> addr = getZ80AddressFromExpression(lex, e, expr.result());
    if (!addr || !expr.eval(*this, lex, m_mmap.getAddress(m_address)))
    {
        error(lex, *e, "Invalid start address expression.");
        nextLine(e);
        return false;
    }

    m_options.m_startAddress = *addr;
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
// Label management
//----------------------------------------------------------------------------------------------------------------------

Labels Assembler::getLabels() const
{
    Labels labels;
    for (const auto& si : m_symbolTable)
    {
        labels.emplace_back(make_pair((const char *)m_lexSymbols.get(si.first), si.second.m_addr));
    }

    sort(labels.begin(), labels.end(), [](const auto& p1, const auto& p2) -> bool {
        return p1.second < p2.second;
    });

    return labels;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
