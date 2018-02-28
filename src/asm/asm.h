//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <asm/lex.h>
#include <asm/stringtable.h>

//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

class AssemblerWindow;

class Assembler
{
public:
    Assembler(AssemblerWindow& window, std::string initialFile);

    void output(const std::string& msg);
    void addError();
    int numErrors() const { return m_numErrors; }

    i64 getSymbol(const u8* start, const u8* end);

private:
    void parse(std::string initialFile);
    void summarise();

private:
    map<string, Lex> m_sessions;
    AssemblerWindow& m_assemblerWindow;
    int m_numErrors;
    StringTable m_symbols;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
