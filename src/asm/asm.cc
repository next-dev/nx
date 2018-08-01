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
    NX_ASSERT(pass > 0);
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

ExprValue::ExprValue(const ExprValue& other)
    : m_type(other.m_type)
    , m_value(other.m_value)
{

}

ExprValue& ExprValue::operator= (const ExprValue& other)
{
    m_type = other.m_type;
    m_value = other.m_value;
    return *this;
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
    SubExpr(const Spectrum& speccy)
        : m_speccy(speccy)
    {}

    ExprValue operator() (i64 x, i64 y)             { return ExprValue(x - y); }
    ExprValue operator() (MemAddr a, i64 x)         { return ExprValue(a - (int)x); }
    ExprValue operator() (i64 x, MemAddr a)         { return m_speccy.isZ80Address(a) ? ExprValue(x - (int)(u16)m_speccy.convertAddress(a)) : ExprValue(x); }
    ExprValue operator() (MemAddr a, MemAddr b)     { return ExprValue(a - b); }

    const Spectrum& m_speccy;
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

 ExprValue ExprValue::opAdd(const ExprValue& a, const ExprValue& b)
{
    return visit(AddExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opSub(const Spectrum& speccy, const ExprValue& a, const ExprValue& b)
{
    return visit(SubExpr(speccy), a.m_value, b.m_value);
}

 ExprValue ExprValue::opMul(const ExprValue& a, const ExprValue& b)
{
    return visit(MulExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opDiv(const ExprValue& a, const ExprValue& b)
{
    return visit(DivExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opMod(const ExprValue& a, const ExprValue& b)
{
    return visit(ModExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opOr(const ExprValue& a, const ExprValue& b)
{
    return visit(OrExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opAnd(const ExprValue& a, const ExprValue& b)
{
    return visit(AndExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opXor(const ExprValue& a, const ExprValue& b)
{
    return visit(XorExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opShiftLeft(const ExprValue& a, const ExprValue& b)
{
    return visit(ShiftLeftExpr(), a.m_value, b.m_value);
}

 ExprValue ExprValue::opShiftRight(const ExprValue& a, const ExprValue& b)
{
    return visit(ShiftRightExpr(), a.m_value, b.m_value);
}

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
                line += stringFormat(": {0}", m_eval.getSymbols().get(el.m_symbol));
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
    m_eval.enumerateLabels([this, &symbols](string name, MemAddr addr) {
        string addressString = m_speccy.addressName(addr);
        symbols.emplace_back(name.substr(0, min(name.size(), size_t(16))), addressString);
    });

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
    m_eval.clear();
    m_mmap.clear(m_speccy);
    m_address = 0;

    //
    // Set up the assembler
    //
    m_fileStack.emplace_back(sourceName);

    //
    // Assembler
    //
    if (assemble(data, sourceName))
    {
        switch (m_options.m_output)
        {
        case Options::Output::Memory:
            m_mmap.upload(m_speccy);
            break;

        case Options::Output::Null:
            // Do nothing.
            break;

        default:
            NX_ASSERT(0);
        }
    }

    output("");
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
    m_errors.reset();

    //
    // Lexical Analysis
    //
    m_sessions[sourceName] = Lex();
    currentLex().parse(m_errors, m_eval.getSymbols(), std::move(data), sourceName);

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
            currentLex().parse(m_errors, m_eval.getSymbols(), std::move(data), fn);

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
    // Flush the errors we've had so far
    for (const auto& line : m_errors.getOutput())
    {
        m_assemblerWindow.output(line);
    }
    m_errors.clearOutput();
    m_assemblerWindow.output(msg);
}

//----------------------------------------------------------------------------------------------------------------------
// Symbol table
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::addLabel(i64 symbol, MemAddr address)
{
    return m_eval.addLabel(symbol, address);
}

bool Assembler::addValue(i64 symbol, ExprValue value)
{
    return m_eval.addValue(symbol, value);
}

optional<MemAddr> Assembler::lookUpLabel(i64 symbol) const
{
    return m_eval.getLabel(symbol);
}

optional<ExprValue> Assembler::lookUpValue(i64 symbol) const
{
    return m_eval.getValue(symbol);
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

        case '$':
            pass = e->m_type == T::Symbol;
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
    m_errors.error(lex, *e, "Invalid instruction.");
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
    m_errors.error(lex, *e, (msg));             \
    nextLine(e);                                \
    buildResult = false;                        \
    } while (0)

#define FAIL_D(msg)                             \
    do {                                        \
    m_errors.error(lex, *directiveE, (msg));    \
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
            if ((expr.getType() == ExprValue::Type::Integer) && (expr.getInteger() < 0x10000))
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
        a = expr.getAddress();
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
        const Lex::Element* directiveE = e;

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
                        if (auto result = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); result)
                        {
                            optional<MemAddr> a = getZ80AddressFromExpression(lex, e, *result);

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
            case T::DEFB:
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
                        string str = (const char *)m_eval.getSymbols().get(e->m_symbol);
                        int strLen = int(m_eval.getSymbols().length(e->m_symbol));
                        m_address += strLen;
                        ++e;
                    }
                    else
                    {
                        FAIL("Invalid argument to a DEFB directive.");
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
            case T::DEFW:
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
                        FAIL("Invalid argument to a DEFW directive.");
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

            case T::DS:
            case T::DEFS:
                {
                    ++e;
                    const Lex::Element* outE = 0;
                    if (expectExpression(lex, e, &outE))
                    {
                        MemAddr addr = m_mmap.getAddress(m_address);
                        if (auto expr = m_eval.parseExpression(lex, m_errors, m_speccy, e, addr); expr)
                        {
                            if (expr->getType() == ExprValue::Type::Integer)
                            {
                                NX_ASSERT(m_speccy.isZ80Address(addr));
                                i64 z80Addr = (i64)(u16)m_speccy.convertAddress(addr);
                                if (z80Addr + expr->getInteger() >= 65536)
                                {
                                    FAIL("Space is too large.");
                                }

                                m_address += (int)expr->getInteger();
                            }
                            else
                            {
                                FAIL("Expression must be an integer.");
                            }
                        }
                        else
                        {
                            FAIL("Invalid expression.");
                        }
                    }
                    else
                    {
                        FAIL("Expected expression to define space.");
                    }

                    symbolToAdd = true;
                    if (e->m_type != T::Newline)
                    {
                        FAIL("Invalid DEFS statement.  Expected a newline");
                    }
                }
                break;

            case T::LOAD:
                ++e;
                if (e->m_type == T::String)
                {
                    string fileName = (const char *)m_eval.getSymbols().get(e->m_symbol);
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
            FAIL_D("Invalid instruction or directive.");
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
                if (!addLabel(symbol, m_mmap.getAddress(symAddress)))
                {
                    // #todo: Output the original line where it is defined.
                    m_errors.error(lex, *e, "Symbol already defined.");
                    buildResult = false;
                }
            }
            else
            {
                m_errors.error(lex, *e, "Address space overrun.  There is not enough space to assemble in this area section.");
            }
        }
    }

    return buildResult;
}

#undef FAIL
#undef FAIL_D

#define PARSE(n, format) if (expect(lex, e, (format), outE)) return (n)
#define CHECK_PARSE(n, format) PARSE(n, format); return invalidInstruction(lex, e, outE)

int Assembler::assembleInstruction1(Lex& lex, const Lex::Element* e, const Lex::Element** outE)
{
    using T = Lex::Element::Type;
    NX_ASSERT(e->m_type > T::_KEYWORDS && e->m_type < T::_END_OPCODES);

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
        m_errors.error(lex, *e, "Unimplemented instruction.");
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
// Pass 2
//----------------------------------------------------------------------------------------------------------------------

#define FAIL(msg)                                   \
    do {                                            \
        m_errors.error(lex, *e, (msg));                      \
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
            NX_ASSERT(!outE || !count || (count == actualCount));
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
            case T::DEFB:
                buildResult = doDb(lex, ++e);
                break;

            case T::DW:
            case T::DEFW:
                buildResult = doDw(lex, ++e);
                break;

            case T::DS:
            case T::DEFS:
                buildResult = doDs(lex, ++e);
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
    ExprValue v = op.expr;

    if (v.getType() == ExprValue::Type::Integer)
    {
        if (v.getInteger() < a || v.getInteger() > b)
        {
            m_errors.error(lex, *e, stringFormat("Integer expression out of range.  Must be be between {0} and {1}.", a, b));
            return false;
        }
    }
    else if (v.getType() == ExprValue::Type::Address && b == 0xffff)
    {
        MemAddr addr(v.getAddress());
        if (!m_speccy.isZ80Address(addr))
        {
            m_errors.error(lex, *e, stringFormat("Address is not in current Z80 view, and so cannot be converted to a 16-bit value."));
            return false;
        }
    }
    else
    {
        m_errors.error(lex, *e, "Invalid expression type.  Expecting an integer expression.");
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
    m_errors.error(lex, *s, "Unimplemented opcode.");    \
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
        op16 = make16(lex, *e, m_speccy, srcOp.expr);                   \
    } while(0)

#define DST_OP8() do {                                                  \
        opSize = 1;                                                     \
        op8 = (dstOp.expr.r8());                                        \
    } while(0)

#define DST_OP16() do {                                                 \
        opSize = 2;                                                     \
        op16 = make16(lex, *e, m_speccy, dstOp.expr);                   \
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
            m_errors.error(lex, *srcE, "Cannot have both IX and IY registers in same instruction.");
            return 0;
        }
        break;

    case OperandType::IY_Expression:
    case OperandType::IY:
    case OperandType::IYH:
    case OperandType::IYL:
        if (indexPrefix == 0xdd)
        {
            m_errors.error(lex, *srcE, "Cannot have both IX and IY registers in same instruction.");
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
        NX_ASSERT(!addressIndex);
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
        NX_ASSERT(!addressIndex);
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
            NX_ASSERT(srcOp.type == OperandType::None);
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
            NX_ASSERT(srcOp.type == OperandType::None);
            XYZ(2, alu(opCode), r(dstOp.type));
            break;

        case OperandType::Expression:
            CHECK8_DST();
            XYZ(3, alu(opCode), 6);
            DST_OP8();
            break;

        case OperandType::HL:
            NX_ASSERT(opCode == T::SBC || opCode == T::ADC);
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
            if (dstOp.expr.getType() != ExprValue::Type::Integer ||
                (dstOp.expr.getInteger() < 0 || dstOp.expr.getInteger() > 7))
            {
                m_errors.error(lex, *dstE, "Invalid bit index.  Must be 0-7.");
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
        else
        {
            return nullptr;
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
                    m_errors.error(lex, *dstE, "Index offsets are not allowed in JP instructions.  Remove the offset.");
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
            else
            {
                return nullptr;
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
            else
            {
                return nullptr;
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
        if (dstOp.expr.getType() != ExprValue::Type::Integer ||
            (dstOp.expr.getInteger() < 0 || dstOp.expr.getInteger() > 0x56) ||
            (dstOp.expr.getInteger() % 8) != 0)
        {
            m_errors.error(lex, *dstE, "Invalid value for RST opcode.");
            return nullptr;
        }
        XYZ(3, u8(dstOp.expr.getInteger() / 8), 7);
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
                NX_ASSERT(dstOp.type != OperandType::Address_HL || srcOp.type != OperandType::Address_HL);
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
                NX_ASSERT(srcOp.type == OperandType::AddressedExpression);
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
            NX_ASSERT(srcOp.type == OperandType::A);
            prefix = 0xed;
            XYZ(1, 0, 7);
            break;

        case OperandType::R:                            // LD R,A
            NX_ASSERT(srcOp.type == OperandType::A);
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
            NX_ASSERT(srcOp.type == OperandType::Address_C);
            prefix = 0xed;
            XYZ(1, r(dstOp.type), 0);
            break;

        case OperandType::None:
            NX_ASSERT(srcOp.type == OperandType::Address_C);
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
                if (srcOp.expr.getType() != ExprValue::Type::Integer || srcOp.expr.getInteger() != 0)
                {
                    m_errors.error(lex, *srcE, "Invalid expression for OUT instruction.  Must be 0 or 8-bit register.");
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
            NX_ASSERT(srcOp.type == OperandType::HL);
            XYZ(3, 4, 3);
            break;

        case OperandType::DE:
            NX_ASSERT(srcOp.type == OperandType::HL);
            XYZ(3, 5, 3);
            break;

        default: UNDEFINED();
        }
        break;

    case T::IM:
        if (dstOp.expr.getType() != ExprValue::Type::Integer ||
            (dstOp.expr.getInteger() < 0 || dstOp.expr.getInteger() > 2))
        {
            m_errors.error(lex, *dstE, "Invalid value of IM instruction.  Must be 0-2.");
            return nullptr;
        }
        prefix = 0xed;
        switch (dstOp.expr.getInteger())
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
        if (!emit8(lex, e, indexPrefix)) return nullptr;
        if (prefix)
        {
            // Has $CB has prefix
            if (!emit8(lex, e, prefix)) return nullptr;
            if (!emit8(lex, e, indexOffset)) return nullptr;
            if (!emitXPQZ(lex, e, x, p, q, z)) return nullptr;
        }
        else
        {
            if (!emitXPQZ(lex, e, x, p, q, z)) return nullptr;

            if (addressIndex)
            {
                NX_ASSERT(opSize < 2);
                if (!emit8(lex, e, indexOffset)) return nullptr;
            }

            switch (opSize)
            {
            case 0:                                             break;
            case 1: if (!emit8(lex, e, op8)) return nullptr;    break;
            case 2: if (!emit16(lex, e, op16)) return nullptr;  break;
            default: NX_ASSERT(0);
            }
        }
    }
    else
    {
        if (prefix)
        {
            if (!emit8(lex, e, prefix)) return nullptr;
        }
        if (!emitXPQZ(lex, e, x, p, q, z)) return nullptr;
        switch (opSize)
        {
        case 0:                                                                     break;
        case 1: NX_ASSERT(!prefix);     if (!emit8(lex, e, op8)) return nullptr;    break;
        case 2:                         if (!emit16(lex, e, op16)) return nullptr;  break;
        default: NX_ASSERT(0);
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
        if (auto result = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); result)
        {
            op.expr = *result;
            op.type = OperandType::Expression;
        }
        else
        {
            return 0;
        }
        break;

    case T::OpenParen:
        // Addressed expression or (HL) or (IX/Y...)
        ++e;
        switch ((e++)->m_type)
        {
        case T::C:
            op.type = OperandType::Address_C;
            NX_ASSERT(e->m_type == T::CloseParen);
            break;

        case T::BC:
            op.type = OperandType::Address_BC;
            NX_ASSERT(e->m_type == T::CloseParen);
            break;

        case T::DE:
            op.type = OperandType::Address_DE;
            NX_ASSERT(e->m_type == T::CloseParen);
            break;

        case T::HL:
            op.type = OperandType::Address_HL;
            NX_ASSERT(e->m_type == T::CloseParen);
            break;

        case T::SP:
            op.type = OperandType::Address_SP;
            NX_ASSERT(e->m_type == T::CloseParen);
            break;

        case T::IX:
            if (e->m_type == T::CloseParen)
            {
                // This is just (IX), no expression
                op.expr = ExprValue(i64(0));
            }
            else
            {
                if (auto result = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); result)
                {
                    op.expr = *result;
                }
                else
                {
                    return 0;
                }
            }
            op.type = OperandType::IX_Expression;
            NX_ASSERT(e->m_type == T::CloseParen);
            break;

        case T::IY:
            if (e->m_type == T::CloseParen)
            {
                // This is just (IY), no expression
                op.expr = ExprValue(i64(0));
            }
            else
            {
                if (auto result = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); result)
                {
                    op.expr = *result;
                }
                else
                {
                    return 0;
                }
            }
            op.type = OperandType::IY_Expression;
            NX_ASSERT(e->m_type == T::CloseParen);
            break;

        default:
            // Must be an address expression
            {
                const Lex::Element* startE = e - 2;
                op.type = OperandType::AddressedExpression;
                const Lex::Element* oldE = --e;
                ExpressionEvaluator::skipExpression(e);
                NX_ASSERT(e->m_type == T::CloseParen);
                Lex::Element::Type nextE = (e + 1)->m_type;
                if (nextE == T::Newline || nextE == T::Comma)
                {
                    if (auto result = m_eval.parseExpression(lex, m_errors, m_speccy, oldE, m_mmap.getAddress(m_address)); result)
                    {
                        op.expr = *result;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    // This is an expression, not an addressed expression because there are extraneous
                    // tokens after the close parentheses matching the initial opening one.
                    // Rebuild the expression, this time including the initial parentheses.
                    op.type = OperandType::Expression;
                    e = startE;
                    if (auto result = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); result)
                    {
                        op.expr = *result;
                    }
                    else
                    {
                        return 0;
                    }
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
        NX_ASSERT(0);
    }

    return true;
}

optional<u8> Assembler::calculateDisplacement(Lex& lex, const Lex::Element* e, ExprValue& expr)
{
    // #todo: Handle pages
    MemAddr a0 = m_mmap.getAddress(m_address) + 2;                                  // Current address
    optional<MemAddr> a1 = getZ80AddressFromExpression(lex, e, expr);      // Address we want to go to
    if (a1)
    {
        int d = *a1 - a0;
        if (d < -128 || d > 127)
        {
            m_errors.error(lex, *e, stringFormat("Relative jump of {0} is too far.  Distance must be between -128 and +127.", d));
            return {};
        }

        return u8(d);
    }
    else
    {
        m_errors.error(lex, *e, "Invalid expression for displacement value.");
    }

    return {};
}

optional<ExprValue> Assembler::calculateExpression(const vector<u8>& exprData)
{
    //
    // Lexical analysis on the expression
    //
    Lex lex;
    if (!lex.parse(m_errors, m_eval.getSymbols(), exprData, "<input>")) return {};

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
    if (auto result = m_eval.parseExpression(lex, m_errors, m_speccy, start, MemAddr()); result)
    {
        return *result;
    }
    else
    {
        return {};
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
        NX_ASSERT(0);
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
        NX_ASSERT(0);
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
        NX_ASSERT(0);
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
        NX_ASSERT(0);
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
        NX_ASSERT(0);
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
        NX_ASSERT(0);
    }

    return 0;
}

bool Assembler::emit8(Lex& l, const Lex::Element* e, u8 b)
{
    if (!m_mmap.poke8(m_address, b))
    {
        m_errors.error(l, *e, stringFormat("Assembled into area previously assembled to before (byte @ ${0}).", hexWord(m_speccy.convertAddress(m_mmap.getAddress(m_address)))));
        return false;
    }
    ++m_address;
    return true;
}

bool Assembler::emit16(Lex& l, const Lex::Element* e, u16 w)
{
    if (!m_mmap.poke16(m_address, w))
    {
        m_errors.error(l, *e, stringFormat("Assembled into area previously assembled to before (word @ ${0}).", hexWord(m_speccy.convertAddress(m_mmap.getAddress(m_address)))));
        return false;
    }
    m_address += 2;
    return true;
}

bool Assembler::emitXYZ(Lex& l, const Lex::Element* e, u8 x, u8 y, u8 z)
{
    NX_ASSERT(x < 4);
    NX_ASSERT(y < 8);
    NX_ASSERT(z < 8);
    return emit8(l, e, (x << 6) | (y << 3) | z);
}

bool Assembler::emitXPQZ(Lex& l, const Lex::Element* e, u8 x, u8 p, u8 q, u8 z)
{
    NX_ASSERT(x < 4);
    NX_ASSERT(p < 4);
    NX_ASSERT(q < 2);
    NX_ASSERT(z < 8);
    return emit8(l, e, (x << 6) | (p << 4) | (q << 3) | z);
}

u16 Assembler::make16(Lex& lex, const Lex::Element& e, const Spectrum& speccy, ExprValue result)
{
    switch (result.getType())
    {
    case ExprValue::Type::Integer:
        return result.r16();

    case ExprValue::Type::Address:
        if (speccy.isZ80Address(result.getAddress()))
        {
            return speccy.convertAddress(result.getAddress());
        }
        else
        {
            m_errors.error(lex, e, "Address expression is not viewable from the current Z80 bank configuration.");
            return 0;
        }
        break;
    default:
        m_errors.error(lex, e, "Invalid 16-bit expression.");
        return 0;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Directives
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::doOrg(Lex& lex, const Lex::Element*& e)
{
    using T = Lex::Element::Type;
    MemAddr a;
    const Lex::Element* start = e;

    if (auto exp = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); exp)
    {
        switch (exp->getType())
        {
        case ExprValue::Type::Integer:
            if (exp->getInteger() < 0 || exp->getInteger() > 0xffff)
            {
                m_errors.error(lex, *start, "Z80 address out of range.");
                nextLine(e);
                return false;
            }
            a = m_speccy.convertAddress(Z80MemAddr((u16)exp->getInteger()));
            break;
            
        case ExprValue::Type::Address:
            a = exp->getAddress();
            if (!m_speccy.isZ80Address(a))
            {
                m_errors.error(lex, *start, "Only Z80 visible addresses allowed at the moment.");
                return false;
            }
            break;

        default:
            m_errors.error(lex, *start, "Expression does not produce a valid address.");
            nextLine(e);
            return false;
        }

        m_mmap.resetRange();
        m_mmap.addRange(a, m_speccy.convertAddress(Z80MemAddr(0xffff)));
        m_address = 0;
        return true;
    }
    else
    {
        m_errors.error(lex, *start, "Invalid expression.");
        nextLine(e);
        return false;
    }
}

bool Assembler::doEqu(Lex& lex, i64 symbol, const Lex::Element*& e)
{
    using T = Lex::Element::Type;
    const Lex::Element* start = e;

    if (auto expr = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); expr)
    {
        if (!addValue(symbol, *expr))
        {
            m_errors.error(lex, *start, "Variable name already used.");
            nextLine(e);
            return false;
        }
        return true;
    }
    else
    {
        m_errors.error(lex, *start, "Invalid expression.");
        nextLine(e);
        return false;
    }
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
            if (auto expr = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); expr)
            {
                if (expr->getType() != ExprValue::Type::Integer ||
                    (expr->getInteger() < -128 || expr->getInteger() > 255))
                {
                    m_errors.error(lex, *startE, "Byte value is out of range.  Must be -128 to +127 or 0-255.");
                    nextLine(e);
                    return false;
                }

                if (!emit8(lex, e, expr->r8()))
                {
                    nextLine(e);
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else if (e->m_type == T::String)
        {
            const char* str = (const char *)m_eval.getSymbols().get(e->m_symbol);
            const char* end = str + m_eval.getSymbols().length(e->m_symbol);
            for (; str != end; ++str)
            {
                if (!emit8(lex, e, *str))
                {
                    nextLine(e);
                    return false;
                }
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
            if (auto expr = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); expr)
            {
                if (expr->getType() == ExprValue::Type::Integer)
                {
                    if (expr->getInteger() < -32768 || expr->getInteger() > 65535)
                    {
                        m_errors.error(lex, *startE, "Word value is out of range.  Must be -32768 to 65535.");
                        nextLine(e);
                        return false;
                    }

                    if (!emit16(lex, e, expr->r16()))
                    {
                        nextLine(e);
                        return false;
                    }
                }
                else if (expr->getType() == ExprValue::Type::Address)
                {
                    MemAddr addr = expr->getAddress();
                    if (m_speccy.isZ80Address(addr))
                    {
                        if (!emit16(lex, e, m_speccy.convertAddress(addr)))
                        {
                            nextLine(e);
                            return false;
                        }
                    }
                    else
                    {
                        m_errors.error(lex, *e, "Address cannot be converted to a word.");
                        nextLine(e);
                        return false;
                    }
                }
                else
                {
                    m_errors.error(lex, *startE, "Integer expression required.");
                    nextLine(e);
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        if (e->m_type == T::Comma) ++e;
    }

    return true;
}

bool Assembler::doDs(Lex& lex, const Lex::Element*& e)
{
    auto expr = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address));
    NX_ASSERT(expr);
    for (i64 i = 0; i < expr->getInteger(); ++i)
    {
        if (!m_mmap.poke8(m_address++, 0))
        {
            m_errors.error(lex, *e, "Space overlaps previously assembled code or data.");
            nextLine(e);
            return false;
        }
    }

    return true;
}

bool Assembler::doOpt(Lex& lex, const Lex::Element*& e)
{
    using T = Lex::Element::Type;

    i64 startSym = m_eval.getSymbols().addString("start", true);
    i64 outputSym = m_eval.getSymbols().addString("output", true);

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
            m_errors.error(lex, *e, "Invalid option syntax.");
            nextLine(e);
            return false;
        }

        if (option == startSym)
        {
            return doOptStart(lex, e);
        }
        else if (option == outputSym)
        {
            return doOptOutput(lex, e);
        }
        else
        {
            m_errors.error(lex, *e, "Unknown option.");
            nextLine(e);
            return false;
        }
    }
    else
    {
        m_errors.error(lex, *e, "Invalid option syntax.");
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
        m_errors.error(lex, *e, "Syntax error in START option.  Should be \"START:<address>\".");
        nextLine(e);
        return false;
    }

    const Lex::Element* start = e;
    if (auto expr = m_eval.parseExpression(lex, m_errors, m_speccy, e, m_mmap.getAddress(m_address)); expr)
    {
        // #todo: Handle full addresses
        optional<MemAddr> addr = getZ80AddressFromExpression(lex, e, *expr);
        if (!addr)
        {
            m_errors.error(lex, *start, "START option requires an address parameter.");
            nextLine(e);
            return false;
        }

        m_options.m_startAddress = *addr;
        return true;
    }
    else
    {
        m_errors.error(lex, *start, "Invalid start address expression.");
        nextLine(e);
        return false;
    }
}

bool Assembler::doOptOutput(Lex& lex, const Lex::Element*& e)
{
    if (!expect(lex, e, "$"))
    {
        m_errors.error(lex, *e, "Syntax error in OUTPUT option.  Should be \"OUTPUT:<type>\".");
        m_errors.output("Supported types are: MEMORY (default), NULL");
        nextLine(e);
        return false;
    }

    i64 memorySym = m_eval.getSymbols().addString("memory", true);
    i64 nullSym = m_eval.getSymbols().addString("null", true);

    if (e->m_symbol == memorySym)
    {
        m_options.m_output = Options::Output::Memory;
    }
    else if (e->m_symbol == nullSym)
    {
        m_options.m_output = Options::Output::Null;
    }
    else
    {
        m_errors.error(lex, *e, "Unknown output type.  Needs to be MEMORY or NULL.");
        nextLine(e);
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
// Label management
//----------------------------------------------------------------------------------------------------------------------

Labels Assembler::getLabels() const
{
    Labels labels;
    m_eval.enumerateLabels([this, &labels](string name, MemAddr addr) {
        labels.emplace_back(make_pair(name, addr));
    });

    sort(labels.begin(), labels.end(), [](const auto& p1, const auto& p2) -> bool {
        return p1.second < p2.second;
    });

    return labels;
}

void Assembler::setLabels(const Labels& labels)
{
    m_eval.clear();
    for (const auto& l : labels)
    {
        i64 symbol = getSymbol((const u8 *)l.first.data(), (const u8 *)l.first.data() + l.first.size(), true);
        addLabel(symbol, l.second);
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
