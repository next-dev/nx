//----------------------------------------------------------------------------------------------------------------------
// ErrorManager implementation
//----------------------------------------------------------------------------------------------------------------------

#include <asm/errors.h>
#include <utils/format.h>

//----------------------------------------------------------------------------------------------------------------------
// ErrorManager
//----------------------------------------------------------------------------------------------------------------------

void ErrorManager::error(const Lex& l, const Lex::Element& el, const string& message)
{
    Lex::Element::Pos start = el.m_position;
    int length = int(el.m_s1 - el.m_s0);

    m_errors.emplace_back(l.getFileName(), message, start.m_line, start.m_col);

    // Output the error
    string err = stringFormat("!{0}({1}): {2}", l.getFileName(), start.m_line, message);
    m_output.emplace_back(err);

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
    m_output.emplace_back(line);
    line = "!";

    // Print the marker
    for (int j = 0; j < x; ++j) line += ' ';
    line += '^';
    for (int j = 0; j < length - 1; ++j) line += '~';
    m_output.emplace_back(line);
}

void ErrorManager::error(const string& fileName, const string& message, int line, int column)
{
    m_errors.emplace_back(fileName, message, line, column);
}

