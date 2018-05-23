//----------------------------------------------------------------------------------------------------------------------
// Lexical analyser
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/lex.h>
#include <utils/format.h>

#ifdef __APPLE__
#define _strnicmp strncasecmp
#endif

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
    /* 20 */    0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0, //  !"#$%&' ()*+,-./
    /* 30 */    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, // 01234567 89:;<=>?
    /* 40 */    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @ABCDEFG HIJKLMNO
    /* 50 */    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, // PQRSTUVW XYZ[\]^_
    /* 60 */    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // `abcdefg hijklmno
    /* 70 */    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // pqrstuvw xyz{|}~
};

// Keywords
static const char* gKeywords[int(Lex::Element::Type::COUNT) - int(Lex::Element::Type::_KEYWORDS)] =
{
    0,
    "ADC",
    "ADD",
    "AND",
    "BIT",
    "CALL",
    "CCF",
    "CP",
    "CPD",
    "CPDR",
    "CPI",
    "CPIR",
    "CPL",
    "DAA",
    "DEC",
    "DI",
    "DJNZ",
    "EI",
    "EX",
    "EXX",
    "HALT",
    "IM",
    "IN",
    "INC",
    "IND",
    "INDR",
    "INI",
    "INIR",
    "JP",
    "JR",
    "LD",
    "LDD",
    "LDDR",
    "LDI",
    "LDIR",
    "NEG",
    "NOP",
    "OR",
    "OTDR",
    "OTIR",
    "OUT",
    "OUTD",
    "OUTI",
    "POP",
    "PUSH",
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
    "SL1",
    "SRA",
    "SRL",
    "SUB",
    "XOR",
    0,
    "DB",
    "DW",
    "EQU",
    "LOAD",
    "OPT",
    "ORG",
    0,
    "A",
    "AF",
    "AF'",
    "B",
    "BC",
    "C",
    "D",
    "DE",
    "E",
    "H",
    "HL",
    "I",
    "IX",
    "IY",
    "IXH",
    "IXL",
    "IYH",
    "IYL",
    "L",
    "M",
    "NC",
    "NZ",
    "P",
    "PE",
    "PO",
    "R",
    "SP",
    "Z",
};

const char* Lex::getKeywordString(Element::Type type) const
{
    return gKeywords[(int)type - (int)Element::Type::_KEYWORDS];
}

//----------------------------------------------------------------------------------------------------------------------
// Lexer implementation
//----------------------------------------------------------------------------------------------------------------------

Lex::Lex()
{
    // Build the keyword table
    int numKeywords = sizeof(gKeywords) / sizeof(gKeywords[0]);
    // If this asserts, we can't encode a keyword in 8 bits.
    NX_ASSERT(numKeywords < 256);
    m_keywords.fill(0);
    for (int i = 1; i < numKeywords; ++i)
    {
        if (gKeywords[i] == 0) continue;

        u64 h = StringTable::hash(gKeywords[i], true);
        int idx = int(h % kKeywordHashSize);
        // If this asserts, there is too many keywords with the same hashed index (maximum 8)
        NX_ASSERT((m_keywords[idx] & 0xff00000000000000ull) == 0);
        m_keywords[idx] <<= 8;
        m_keywords[idx] += u64(i);
    }
}

bool Lex::parse(Assembler& assembler, const vector<u8>& data, string sourceName)
{
    m_file = data;
    m_fileName = sourceName;
    m_start = m_file.data();
    m_end = m_file.data() + m_file.size();
    m_cursor = m_file.data();
    m_position = Element::Pos{ 0, 1, 1 };
    m_lastPosition = Element::Pos{ 0, 1, 1 };

    bool result = true;

    Element::Type t = Element::Type::Unknown;
    while (t != Element::Type::EndOfFile)
    {
        t = next(assembler);
        if (t == Element::Type::Error) result = false;
    };

    return result;
}

char Lex::nextChar(bool toUpper)
{
    char c;

    m_lastPosition = m_position;
    m_lastCursor = m_cursor;
    if (m_cursor == m_end) return 0;

    c = *m_cursor++;
    ++m_position.m_col;

    // Convert to uppercase.
    if (toUpper && c >= 'a' && c <= 'z') c -= 32;

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
    assembler.addErrorInfo(m_fileName, msg, m_lastPosition.m_line, m_lastPosition.m_col);

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
                // Add a new line if there isn't one already.
                if (m_elements.empty() || m_elements.back().m_type != Element::Type::Newline)
                {
                    buildElemInt(el, Element::Type::Newline, m_lastPosition, 0);
                }
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
        while (gNameChar[c]) c = nextChar();
        ungetChar();

        el.m_s1 = m_cursor;
        u64 h = StringTable::hash((const char *)el.m_s0, (const char *)el.m_s1, true);
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
                    return buildElemInt(el, (Element::Type)((int)Element::Type::_KEYWORDS + index), pos, 0);
                }
            }
        }

        // It's a symbol
        return buildElemSymbol(el, Element::Type::Symbol, pos, assembler.getSymbol(el.m_s0, el.m_s1, true));
    }

    //------------------------------------------------------------------------------------------------------------------
    // Check for strings
    //------------------------------------------------------------------------------------------------------------------

    else if ('"' == c || '\'' == c)
    {
        char delim = c;
        el.m_s0 = m_cursor;
        pos = m_position;
        c = nextChar(false);
        vector<char> s;
        while (c != delim)
        {
            if (0 == c || '\n' == c)
            {
                return error(assembler, "Unterminated string.");
            }

            if (c == '\\')
            {
                c = nextChar(false);
                switch (c)
                {
                case '\\':  s.emplace_back('\\');      break;
                case 'n':   s.emplace_back('\n');      break;
                case 'r':   s.emplace_back('\r');      break;
                case '0':   s.emplace_back('\0');      break;
                case '\'':  s.emplace_back('\'');      break;
                case '"':   s.emplace_back('"');       break;
                case 'x':
                    {
                        i64 t = 0;

                        // First nibble
                        c = nextChar(false);
                        if (!(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'F') && !(c >= 'a' && c <= 'f'))
                        {
                            ungetChar();
                            return error(assembler, "Invalid hexadecimal character in string.");
                        }
                        t = c - '0';
                        if (c >= 'a' && c <= 'f') t -= 32;
                        if (c >= 'A' && c <= 'F') t -= 7;

                        // Second nibble
                        c = nextChar(false);
                        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
                        {
                            t *= 16;
                            t += c - '0';
                            if (c >= 'a' && c <= 'f') t -= 32;
                            if (c >= 'A' && c <= 'F') t -= 7;
                        }
                        else
                        {
                            ungetChar();
                        }

                        s.emplace_back(char(t));
                    }
                    break;
                }
            }
            else
            {
                s.emplace_back(c);
            }
            c = nextChar(false);
        }
        el.m_s1 = m_cursor - 1;

        if ('\'' == c)
        {
            if (s.size() != 1)
            {
                return error(assembler, "Invalid character literal.");
            }
            return buildElemInt(el, Element::Type::Char, pos, int(s[0]));
        }
        else
        {
            return buildElemSymbol(el, Element::Type::String, pos,
                assembler.getSymbol((const u8 *)s.data(), (const u8 *)s.data() + s.size(), false));
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    // Check for integers
    //------------------------------------------------------------------------------------------------------------------

    else if (
        // Check for digits
        (c >= '0' && c <= '9') ||
        ('$' == c) ||
        ('%' == c))
    {
        // Possible number
        int base = 10;

        if ('$' == c)
        {
            base = 16;
            c = nextChar();
            if (!(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'F'))
            {
                // Not a hexadecimal digit, but a reference to current address.
                // However, if there was a previous '-', then that's an error
                ungetChar();
                return buildElemInt(el, Element::Type::Dollar, pos, 0);
            }
        }
        else if ('%' == c)
        {
            base = 2;
            c = nextChar();
        }

        // Should now be parsing digits
        i64 t = 0;
        while ((c >= '0' && c <= '9') || (base == 16 && c >= 'A' && c <= 'F'))
        {
            t *= base;
            int d = (c - '0');
            if (c >= 'A' && c <= 'F') d -= 7;
            if (d >= base)
            {
                return error(assembler, "Invalid number literal.");
            }
            t += d;

            c = nextChar();
        }
        ungetChar();
        el.m_s1 = m_cursor;

        return buildElemInt(el, Element::Type::Integer, pos, t);
    }

    //------------------------------------------------------------------------------------------------------------------
    // Check for operators
    //------------------------------------------------------------------------------------------------------------------

    else if (',' == c) return buildElemInt(el, Element::Type::Comma, pos, 0);
    else if ('(' == c) return buildElemInt(el, Element::Type::OpenParen, pos, 0);
    else if (')' == c) return buildElemInt(el, Element::Type::CloseParen, pos, 0);
    else if ('+' == c) return buildElemInt(el, Element::Type::Plus, pos, 0);
    else if ('-' == c) return buildElemInt(el, Element::Type::Minus, pos, 0);
    else if (':' == c) return buildElemInt(el, Element::Type::Colon, pos, 0);
    else if ('|' == c) return buildElemInt(el, Element::Type::LogicOr, pos, 0);
    else if ('&' == c) return buildElemInt(el, Element::Type::LogicAnd, pos, 0);
    else if ('^' == c) return buildElemInt(el, Element::Type::LogicXor, pos, 0);
    else if ('~' == c) return buildElemInt(el, Element::Type::Tilde, pos, 0);
    else if ('*' == c) return buildElemInt(el, Element::Type::Multiply, pos, 0);
    else if ('/' == c) return buildElemInt(el, Element::Type::Divide, pos, 0);
    else if ('%' == c) return buildElemInt(el, Element::Type::Mod, pos, 0);

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

