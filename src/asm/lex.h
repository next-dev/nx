//----------------------------------------------------------------------------------------------------------------------
// Lexical analyser
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <config.h>
#include <asm/stringtable.h>

#include <map>
#include <vector>

class Lex
{
public:
    Lex();

    void parse(string fileName);

public:
    // Lexical analysis data structures
    struct Element
    {
        enum class Type
        {
            Unknown,
            Error,

            // End of token stream
            EndOfFile,

            // Literals
            Newline,
            Symbol,
            Integer,

            // Operators
            Comma,
            OpenParen,
            CloseParen,
            Dollar,
            Plus,
            Minus,
            Quote,
            Colon,
            LogicOr,
            LogicAnd,
            LogicXor,
            ShiftLeft,
            ShiftRight,
            Tilde,
            Multiply,
            Divide,
            Mod,

            KEYWORDS,

            // Keywords
            A,
            ADC,
            ADD,
            AF,
            AND,
            B,
            BC,
            BIT,
            C,
            CALL,
            CCF,
            CP,
            CPD,
            CPDR,
            CPI,
            CPIR,
            CPL,
            D,
            DAA,
            DE,
            DEC,
            DI,
            DJNZ,
            E,
            EI,
            EQU,
            EX,
            EXX,
            H,
            HALT,
            HL,
            I,
            IM,
            IN,
            INC,
            IND,
            INDR,
            INI,
            INIR,
            IX,
            IY,
            JP,
            JR,
            L,
            LD,
            LDD,
            LDDR,
            LDI,
            LDIR,
            M,
            NC,
            NEG,
            NOP,
            NZ,
            OR,
            ORG,
            OTDR,
            OTIR,
            OUT,
            OUTD,
            OUTI,
            P,
            PE,
            PO,
            POP,
            PUSH,
            R,
            RES,
            RET,
            RETI,
            RETN,
            RL,
            RLA,
            RLC,
            RLCA,
            RLD,
            RR,
            RRA,
            RRC,
            RRCA,
            RRD,
            RST,
            SBC,
            SCF,
            SET,
            SLA,
            SLL,
            SP,
            SRA,
            SRL,
            SUB,
            XOR,
            Z,

            COUNT
        };

        struct Pos
        {
            i64     m_lineOffset;
            i32     m_line;
            i32     m_col;
        };

        Type        m_type;
        Pos         m_position;
        union {
            i64         m_symbol;
            u8*         m_s0;
            u8*         m_s1;
            i64         i;
        };
    };

    const vector<string>& getErrors() const         { return m_ctx.getErrors(); }

private:
    class Context;

    class Session
    {
    public:
        Session();

        void parse(Context& ctx, string fileName);

    private:
        char nextChar();
        void ungetChar();
        Element::Type next(Context& ctx);

        Element::Type error(Context& ctx, const std::string& msg);

    private:
        vector<Element>     m_elements;
        vector<u8>          m_file;
        string              m_fileName;
        const u8*           m_start;
        const u8*           m_end;
        const u8*           m_cursor;
        const u8*           m_lastCursor;
        Element::Pos        m_position;
        Element::Pos        m_lastPosition;

    };

    class Context
    {
    public:
        Context();

        void parse(string fileName);

        void error(const std::string& errorString);
        const vector<string>& getErrors() const { return m_errors; }

    private:
        map<string, Session>    m_sessions;
        vector<string>          m_errors;
    };

    Context     m_ctx;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
