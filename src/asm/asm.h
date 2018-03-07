//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <asm/lex.h>
#include <asm/stringtable.h>
#include <emulator/spectrum.h>

#include <array>
#include <map>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

class AssemblerWindow;

class Assembler
{
public:
    //------------------------------------------------------------------------------------------------------------------
    // Public interface
    //------------------------------------------------------------------------------------------------------------------

    Assembler(AssemblerWindow& window, Spectrum& speccy, std::string initialFile, array<int, 4> pages);

    void output(const std::string& msg);
    void addError();
    int numErrors() const { return m_numErrors; }
    void error(const Lex& l, const Lex::Element& el, const string& message);

    i64 getSymbol(const u8* start, const u8* end);

private:
    //------------------------------------------------------------------------------------------------------------------
    // Internal methods
    //------------------------------------------------------------------------------------------------------------------

    // Generates a vector<Lex::Element> database from a file
    void parse(std::string initialFile);
    void dumpLex(const Lex& l);

    //------------------------------------------------------------------------------------------------------------------
    // Pass 1
    // The first pass figures out the segments, opcodes and label addresses.  No expressions are evaluated at this
    // point.

private:
    //------------------------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------------------------

    map<string, Lex>            m_sessions;
    AssemblerWindow&            m_assemblerWindow;
    Spectrum&                   m_speccy;
    int                         m_numErrors;
    StringTable                 m_symbols;
    array<int, 4>               m_pages;       // Current configuration of pages
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
