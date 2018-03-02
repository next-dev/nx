//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/overlay_asm.h>
#include <utils/format.h>

#define NX_DEBUG_LOG_LEX    (1)

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Assembler::Assembler(AssemblerWindow& window, std::string initialFile)
    : m_assemblerWindow(window)
    , m_numErrors(0)
{
    window.clear();
    parse(initialFile);
}

void Assembler::parse(std::string initialFile)
{
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

            for (const auto& el : m_sessions[initialFile].elements())
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
                    line = stringFormat("{0}: {1}", el.m_position.m_line, m_sessions[initialFile].getKeywordString(el.m_type));
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
                    const vector<u8>& file = m_sessions[initialFile].getFile();
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
#endif // NX_DEBUG_LOG_LEX
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

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
