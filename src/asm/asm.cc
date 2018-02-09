//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#include <asm/asm.h>
#include <asm/overlay_asm.h>
#include <utils/format.h>

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

void Assembler::output(const std::string& msg)
{
    m_assemblerWindow.output(msg);
}

void Assembler::addError()
{
    ++m_numErrors;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
