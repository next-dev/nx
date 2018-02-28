//----------------------------------------------------------------------------------------------------------------------
// Lexical analyser
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/lex.h>
#include <emulator/nxfile.h>
#include <utils/format.h>

//----------------------------------------------------------------------------------------------------------------------
// Lexical tables
//----------------------------------------------------------------------------------------------------------------------

// This table represents the validity of a name (symbol or keyword) character.
//
//      0 = Cannot be found within a name.
//      1 = Can be found within a name.
//      2 = Can be found within a name but not as the initial character.
//
static char gNameChar[128] =
{
    //          00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f // Characters
    /* 00 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
    /* 10 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //
    /* 20 */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  !"#$%&' ()*+,-./
    /* 30 */    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, // 01234567 89:;<=>?
    /* 40 */    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @ABCDEFG HIJKLMNO
    /* 50 */    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, // PQRSTUVW XYZ[\]^_
    /* 60 */    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // `abcdefg hijklmno
    /* 70 */    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // pqrstuvw xyz{|}~
};

// Keywords
static const char* gKeywords[int(Lex::Element::Type::COUNT) - int(Lex::Element::Type::KEYWORDS) - 1] =
{
    "A",
    "ADC",
    "ADD",
    "AF",
    "AND",
    "B",
    "BC",
    "BIT",
    "C",
    "CALL",
    "CCF",
    "CP",
    "CPD",
    "CPDR",
    "CPI",
    "CPIR",
    "CPL",
    "D",
    "DAA",
    "DE",
    "DEC",
    "DI",
    "DJNZ",
    "E",
    "EI",
    "EQU",
    "EX",
    "EXX",
    "H",
    "HALT",
    "HL",
    "I",
    "IM",
    "IN",
    "INC",
    "IND",
    "INDR",
    "INI",
    "INIR",
    "IX",
    "IY",
    "JP",
    "JR",
    "L",
    "LD",
    "LDD",
    "LDDR",
    "LDI",
    "LDIR",
    "M",
    "NC",
    "NEG",
    "NOP",
    "NZ",
    "OR",
    "ORG",
    "OTDR",
    "OTIR",
    "OUT",
    "OUTD",
    "OUTI",
    "P",
    "PE",
    "PO",
    "POP",
    "PUSH",
    "R",
    "RES",
    "RET",
    "RETI",
    "RETN",
    "RL",
    "RLA",
    "RLC",
    "RLCA",
    "RLD",
    "RR",
    "RRA",
    "RRC",
    "RRCA",
    "RRD",
    "RST",
    "SBC",
    "SCF",
    "SET",
    "SLA",
    "SLL",
    "SP",
    "SRA",
    "SRL",
    "SUB",
    "XOR",
    "Z",
};

const char* Lex::getKeywordString(Element::Type type)
{
    return gKeywords[(int)type - (int)Element::Type::KEYWORDS - 1];
}

//----------------------------------------------------------------------------------------------------------------------
// Lexer implementation
//----------------------------------------------------------------------------------------------------------------------

Lex::Lex()
{
    // Build the keyword table
    int numKeywords = sizeof(gKeywords) / sizeof(gKeywords[0]);
    // If this asserts, we can't encode a keyword in 8 bits.
    assert(numKeywords < 256);
    m_keywords.fill(0);
    for (int i = 0; i < numKeywords; ++i)
    {
        u64 h = StringTable::hash(gKeywords[i]);
        int idx = int(h % kKeywordHashSize);
        // If this asserts, there is too many keywords with the same hashed index (maximum 8)
        assert((m_keywords[idx] & 0xff00000000000000ull) == 0);
        m_keywords[idx] <<= 8;
        m_keywords[idx] += u64(i);
    }
}

void Lex::parse(Assembler& assembler, string fileName)
{
    m_file = NxFile::loadFile(fileName);
    m_fileName = fileName;
    m_start = m_file.data();
    m_end = m_file.data() + m_file.size();
    m_cursor = m_file.data();
    m_position = Element::Pos{ 0, 1, 1 };
    m_lastPosition = Element::Pos{ 0, 1, 1 };

    while (next(assembler) != Element::Type::EndOfFile) ;
}

char Lex::nextChar()
{
    char c;

    m_lastPosition = m_position;
    m_lastCursor = m_cursor;
    if (m_cursor == m_end) return 0;

    c = *m_cursor++;
    ++m_position.m_col;

    // Convert to uppercase.
    if (c >= 'a' && c <= 'z') c -= 32;

    // Check for newlines
    if ('\r' == c || '\n' == c)
    {
        ++m_position.m_line;
        m_position.m_col = 1;
        if (c == '\r')
        {
            // Handle Mac or Windows newlines
            if ((m_cursor < m_end) && (*m_cursor == '\n'))
            {
                // This is CRLF (Windows)
                ++m_cursor;
            }
            // Either way, make sure the character is always '\n'.
            c = '\n';
        }
        m_position.m_lineOffset = m_cursor - m_start;
    }

    return c;
}

void Lex::ungetChar()
{
    m_position = m_lastPosition;
    m_cursor = m_lastCursor;
}

Lex::Element::Type Lex::error(Assembler& assembler, const std::string& msg)
{
    assembler.output(stringFormat("!{0}({1}): Lexical Error: {2}", m_fileName, m_lastPosition.m_line, msg));

    i32 x = m_lastPosition.m_col - 1;
    const u8* lineStart = m_file.data() + m_lastPosition.m_lineOffset;
    const u8* fileEnd = m_file.data() + m_file.size();

    // Print the line that token resides in
    const u8* p = lineStart;
    while ((p < fileEnd) && (*p != '\r') && (*p != '\n')) ++p;
    string line(lineStart, p);
    assembler.output(line);

    // Print the cursor point where the error is
    line.clear();
    for (int j = 0; j < x; ++j) line += ' ';
    line += '^';
    assembler.output(line);
    assembler.addError();

    return Element::Type::Error;
}

Lex::Element::Type Lex::next(Assembler& assembler)
{
    char c = nextChar();

    //
    // Find the first meaningful character (but handle end of file too!).
    //
    for (;;)
    {
        if (0 == c)
        {
            // End of file

            // Make sure that the stream of tokens has an EOF token.
            if (m_elements.empty() || m_elements.back().m_type != Element::Type::EndOfFile)
            {
                Element el;
                el.m_s0 = m_cursor - 1;
                el.m_s1 = m_cursor;
                buildElemInt(el, Element::Type::EndOfFile, m_lastPosition, 0);
            }

            return Element::Type::EndOfFile;
        }

        if ('\n' != c && iswspace(c))
        {
            // Keep skipping whitespace
            c = nextChar();
            continue;
        }

        // Check for comments
        if (';' == c)
        {
            while (c != 0 && c != '\n') c = nextChar();
            continue;
        }

        break;
    }

    Element el;
    Element::Pos pos = m_lastPosition;
    el.m_s0 = m_cursor - 1;
    el.m_s1 = m_cursor;

    //------------------------------------------------------------------------------------------------------------------
    // Check for new line
    //------------------------------------------------------------------------------------------------------------------

    if ('\n' == c)
    {
        return buildElemInt(el, Element::Type::Newline, pos, 0);
    }

    //------------------------------------------------------------------------------------------------------------------
    // Check for symbols and keywords
    //------------------------------------------------------------------------------------------------------------------

    else if (gNameChar[c] == 1)
    {
        // Possible symbol or keyword
        int l = 1;

        while (gNameChar[c]) c = nextChar();
        ungetChar();

        el.m_s1 = m_cursor;
        u64 h = StringTable::hash((const char *)el.m_s0, (const char *)el.m_s1);
        u64 tokens = m_keywords[h % kKeywordHashSize];
        i64 sizeToken = (el.m_s1 - el.m_s0);

        while (tokens != 0)
        {
            int index = (tokens & 0xff);
            tokens >>= 8;

            if (strlen(gKeywords[index]) == sizeToken)
            {
                // Length of source string and current keyword match.
                if (_strnicmp((const char *)el.m_s0, gKeywords[index], sizeToken) == 0)
                {
                    // It is a keyword
                    return buildElemInt(el, (Element::Type)((int)Element::Type::KEYWORDS + 1 + index), pos, 0);
                }
            }
        }

        // It's a symbol
        return buildElemSymbol(el, Element::Type::Symbol, pos, assembler.getSymbol(el.m_s0, el.m_s1));
    }

    //------------------------------------------------------------------------------------------------------------------
    // Unknown token
    //------------------------------------------------------------------------------------------------------------------

    else {
        buildElemInt(el, Element::Type::Unknown, pos, 0);
        return error(assembler, "Unknown token");
    }
}

Lex::Element::Type Lex::buildElemInt(Element& el, Element::Type type, Element::Pos pos, i64 integer)
{
    el.m_type = type;
    el.m_position = pos;
    el.m_integer = integer;
    m_elements.push_back(el);
    return type;
}

Lex::Element::Type Lex::buildElemSymbol(Element& el, Element::Type type, Element::Pos pos, i64 symbol)
{
    el.m_type = type;
    el.m_position = pos;
    el.m_symbol = symbol;
    m_elements.push_back(el);
    return type;
}

