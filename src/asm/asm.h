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
        Address     m_address;
        u16         m_z80Address;       // If m_address is INVALID_ADDRESS, this value has meaning
        i64         m_fileName;
        int         m_line;
    };

    struct Expression
    {
        enum class Type
        {
            Unknown,
            Integer,        // n, $nn, $nnnn
            Symbol,         // label that needs to be looked up
            Address,        // Refers to a paged address - needs functions PAGE() and OFFSET() to convert to ints.
        };

        enum class OpType
        {
            None
        };

        struct Sub
        {
            Type            m_type;
            i64             m_value;
            Lex::Element    m_elem;

            Sub(Type type, i64 value, const Lex::Element& el) : m_type(type), m_value(value), m_elem(el) {}
        };

        struct Op
        {
            OpType          m_opType;
            Lex::Element    m_elem;

            Op(OpType type, const Lex::Element& el) : m_opType(type), m_elem(el) {}
        };

        const Lex*      m_lex;
        vector<Sub>     m_values;
        vector<Op>      m_ops;

        bool isExpression() const { return (m_values.size() != 0); }
        bool ready() const { return (m_values.size() == 1) && (m_ops.size() == 0) && m_values[0].m_type == Type::Integer; }
        bool check8bit(Assembler& assembler, const Lex& l, const Lex::Element& el) const;
        bool checkSigned8bit(Assembler& assembler, const Lex& l, const Lex::Element& el) const;
        bool check16bit(Assembler& assembler, const Lex& l, const Lex::Element& el) const;

        u8 get8Bit() const;
        u16 get16Bit() const;

        void addInteger(i64 i, const Lex::Element& el) {
            m_values.emplace_back(Type::Integer, i, el);
        }
        void addSymbol(i64 sym, const Lex::Element& el) { m_values.emplace_back(Type::Symbol, sym, el); }
        void addOp(OpType op, const Lex::Element& el) { m_ops.emplace_back(op, el); }
    };

    struct Instruction
    {
        Lex::Element::Type  m_opCode;
        Lex::Element        m_opCodeElem;
        Expression          m_dst;
        Expression          m_src;
        Lex::Element        m_dstElem;
        Lex::Element        m_srcElem;
    };

    struct DeferredExpression
    {
        enum class Type
        {
            Bit8,
            SignedBit8,
            Bit16,
        };
        Type                m_requiredType;
        Expression          m_expr;
        Address             m_addr;

        DeferredExpression(Type type, const Expression& expr, Address addr)
            : m_requiredType(type)
            , m_expr(expr)
            , m_addr(addr)
        {}
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
    void invalidSrcOperand(const Lex& l, const Instruction& inst);
    void invalidDstOperand(const Lex& l, const Instruction& inst);
    bool emit8(u8 byte);
    bool emit16(u16 word);
    bool emitXYZ(u8 x, u8 y, u8 z);
    bool emitXPQZ(u8 x, u8 p, u8 q, u8 z);

    // Decoding utilities (see http://www.z80.info/decoding.htm)
    u8 rp(Lex::Element::Type t);
    u8 rp2(Lex::Element::Type t);

    // Pass 2 utilities.  They calculate the expression, and if they're read will emit them, otherwise emit 0, and
    // store a reference to them for later calculation during pass 2.  If they are ready but invalid size, an error
    // will occur and 0 will be written out too.
    void do8bit(const Expression& exp, const Lex& l, const Lex::Element& el);
    void doSigned8bit(const Expression& exp, const Lex& l, const Lex::Element& el);
    void do16bit(const Expression& exp, const Lex& l, const Lex::Element& el);

    // Expressions
    bool eval(Expression& expr);


private:
    //------------------------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------------------------

    map<string, Lex>            m_sessions;
    AssemblerWindow&            m_assemblerWindow;
    Spectrum&                   m_speccy;
    int                         m_numErrors;
    StringTable                 m_symbols;
    StringTable                 m_files;

    array<int, 4>               m_pages;       // Current configuration of pages
    vector<Block>               m_blocks;       // Assembled block
    map<i64, Ref>               m_symbolTable;
    size_t                      m_blockIndex;
    Address                     m_address;

    vector<DeferredExpression>  m_deferredExprs;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
