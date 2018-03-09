//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/overlay_asm.h>
#include <emulator/nxfile.h>
#include <emulator/spectrum.h>
#include <utils/format.h>

#define NX_DEBUG_LOG_LEX    (1)

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
{
    window.clear();
    startAssembly(data, sourceName);
}

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
// Pass 1
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::pass1(Lex& lex, const vector<Lex::Element>& elems)
{
    using T = Lex::Element::Type;
    const Lex::Element* e = elems.data();
    i64 symbol;

    while (e->m_type != T::EndOfFile)
    {
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
            error(lex, *e, "Unimplemented instruction.");
            while (e->m_type != T::Newline) ++e;
            ++e;
            return false;
        }
        else if (e->m_type > T::_END_OPCODES && e->m_type < T::_END_DIRECTIVES)
        {
            // It's a possible directive
            error(lex, *e, "Unimplemented directive.");
            while (e->m_type != T::Newline) ++e;
            ++e;
            return false;
        }
        else if (e->m_type != T::Newline)
        {
            error(lex, *e, "Invalid instruction or directive.");
            while (e->m_type != T::Newline) ++e;
            ++e;
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
// Pass 2
//----------------------------------------------------------------------------------------------------------------------

bool Assembler::pass2(Lex& lex, const vector<Lex::Element>& elems)
{
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
