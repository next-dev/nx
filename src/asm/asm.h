//----------------------------------------------------------------------------------------------------------------------
// Assembler
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <asm/lex.h>
#include <asm/stringtable.h>

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
    Assembler(AssemblerWindow& window, std::string initialFile, array<int, 4> pages);

    void output(const std::string& msg);
    void addError();
    int numErrors() const { return m_numErrors; }
    void error(const Lex& l, const Lex::Element& el, const string& message);

    i64 getSymbol(const u8* start, const u8* end);

private:
    void parse(std::string initialFile);
    void summarise();
    void dumpLex(const Lex& l);
    void assemble(const Lex& l, const vector<Lex::Element>& elems);

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

    Address addressUp(u16 addr);    // Convert a 64K address space address ($nnnn) to a full address ($nn:$nnnn)
    Address incAddress(Address addr, int n);
    Address getAddress();

    static u64 getFullAddress(Address addr) {
        return u64(addr.m_page * KB(16)) + u64(addr.m_offset);
    }
    static Address getPagedAddress(u64 fullAddr) {
        return { u8(fullAddr / KB(16)), u16(fullAddr % KB(16)) };
    }

    struct Block
    {
        Address     m_address;      // Initial address of block
        vector<u8>  m_bytes;

        Block(Address address) : m_address(address) {}

        bool operator< (const Block& other) const { return m_address < other.m_address; }
        bool operator== (const Block& other) const { return m_address == other.m_address; }
    };

    Block& openBlock();
    Block& block();

    struct Ref
    {
        size_t      m_blockIndex;
        size_t      m_blockOffset;
        i64         m_fileName;
        int         m_line;
    };

    Ref getRef(const string& fileName, int line);

private:
    struct Operand
    {
        enum class Type
        {
        };

        Type m_type;
        union {

        };
    };

    struct Instruction
    {
        enum class Opcode
        {

        };

        Opcode      m_opCode;
        Operand     m_dst;
        Operand     m_src;
    };

    //
    // Assembling functions
    //
    void assembleLine(const Lex& l, const Lex::Element* e, const Lex::Element* end);

private:
    map<string, Lex>    m_sessions;
    AssemblerWindow&    m_assemblerWindow;
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
