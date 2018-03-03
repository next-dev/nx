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
    // Internal structures
    //------------------------------------------------------------------------------------------------------------------
    
    struct Address
    {
        u8          m_page;
        u16         m_offset;

        bool operator< (const Address& other) const
        {
            if (m_page < other.m_page) return true;
            if (m_page == other.m_page && m_offset < other.m_offset) return true;
            return false;
        }

        bool operator== (const Address& other) const 
        {
            return (m_page == other.m_page && m_offset == other.m_offset);
        }
    };

    static const Address INVALID_ADDRESS;

    struct Block
    {
        Address     m_address;      // Initial address of block
        bool        m_crossPage;
        vector<u8>  m_bytes;

        Block(Address address) : m_address(address), m_crossPage(false) {}

        bool operator< (const Block& other) const { return m_address < other.m_address; }
        bool operator== (const Block& other) const { return m_address == other.m_address; }
    };

    struct Ref
    {
        size_t      m_blockIndex;
        size_t      m_blockOffset;
        i64         m_fileName;
        int         m_line;
    };

    struct Expression
    {
        enum class Type
        {
            integer,        // n, $nn, $nnnn
            address,        // (n, $nn, $nnnn)
            symbol,         // label that needs to be looked up
            compound,
        };

        Type                m_type;
    };

    struct Operand
    {
        Lex::Element::Type  m_type;
        Expression          m_exp;      // Optional expression
    };

    struct Instruction
    {
        Lex::Element::Type  m_opCode;
        Lex::Element        m_opCodeElem;
        Operand             m_dst;
        Operand             m_src;
        Lex::Element        m_dstElem;
        Lex::Element        m_srcElem;
    };

private:
    //------------------------------------------------------------------------------------------------------------------
    // Internal methods
    //------------------------------------------------------------------------------------------------------------------

    void parse(std::string initialFile);
    void summarise();
    void dumpLex(const Lex& l);
    void assemble(const Lex& l, const vector<Lex::Element>& elems);

    Address addressUp(u16 addr);    // Convert a 64K address space address ($nnnn) to a full address ($nn:$nnnn)
    Address incAddress(Address addr, const Block& currentBlock, int n);         // Increment address, and test for page boundary crossings or out of memory errors.
    Address getAddress();
    bool validAddress() const;      // Return true if the current address is valid (we haven't run over a boundary)

    // $nn:$nnnn --> $nnnnnnnnnnnnnnnn
    static u64 getFullAddress(Address addr) {
        return u64(addr.m_page * KB(16)) + u64(addr.m_offset);
    }

    // $nnnnnnnnnnnnnnnn -> $nn:$nnnn
    static Address getPagedAddress(u64 fullAddr) {
        return { u8(fullAddr / kPageSize), u16(fullAddr % kPageSize) };
    }

    // $nn:$nnnn -> $nnnn / INVALID_ADDRESS
    u16 getZ80Address(Address addr);

    Block& openBlock();
    Block& block();

    Ref getRef(const string& fileName, int line);

    //
    // Assembling functions
    //
    void assembleLine(const Lex& l, const Lex::Element* e, const Lex::Element* end);
    bool buildInstruction(const Lex& l, const Lex::Element* e, const Lex::Element* end, Instruction& inst);
    void assembleInstruction(const Lex& l, const Instruction& inst);

    bool expectOperands(const Lex& l, const Instruction& inst, int numOps);
    bool emit8(u8 byte);
    bool emit16(u16 word);

private:
    //------------------------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------------------------

    map<string, Lex>    m_sessions;
    AssemblerWindow&    m_assemblerWindow;
    Spectrum&           m_speccy;
    int                 m_numErrors;
    StringTable         m_symbols;
    StringTable         m_files;

    array<int, 4>       m_pages;       // Current configuration of pages
    vector<Block>       m_blocks;       // Assembled block
    map<i64, Ref>       m_symbolTable;
    size_t              m_blockIndex;
    Address             m_address;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
