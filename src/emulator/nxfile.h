//----------------------------------------------------------------------------------------------------------------------
// NX File format
//
// All 16-bit/32-bit values are little endian.
//
// FILE FORMAT:
//
//      Offset  Length  Description
//      0       4       'NX00'
//      8       ?       Block 0+
//
// BLOCK FORMAT:
//
//      Offset  Length  Description
//      0       4       '????' - Block type
//      4       4       Length of block
//      8       ?       Block data
//
// BLOCK TYPES & FORMATS:
//
//      MODL (length = 1)
//          Offset  Length  Description
//          0       1       Model (0=48K, 1=128K, 2=+2, 3=+2A, 4=+3, 5=Next)
//
//          NOTE: The following blocks are expected with regard to the model selection:
//              0   48K     Expects: SN48, RM48
//              1   128K    Expects: SN48, S128, R128
//              2   +2      "        "     "     "
//              3   +2A     Expects: SN48, S128, SPL3, R128
//              4   +3      "        "     "     "     "
//              5   Next    TBD
//
//      SN48 (length = 36)
//          Offset  Length  Description
//          0       2       Contents of AF
//          2       2       Contents of BC
//          4       2       Contents of DE
//          6       2       Contents of HL
//          8       2       Contents of AF'
//          10      2       Contents of BC'
//          12      2       Contents of DE'
//          14      2       Contents of HL'
//          16      2       Contents of IX
//          18      2       Contents of IY
//          20      2       Contents of SP
//          22      2       Contents of PC
//          24      2       Contents of IR
//          26      2       Contents of WZ
//          28      1       Interrupt mode
//          29      1       IFF1
//          30      1       IFF2
//          31      1       Border colour
//          32      4       T-state
//
//      S128 (length = 16)
//          Offset  Length  Description
//          0       1       Last write to I/O port $7ffd
//          1       15      15 register values of the AY-3-8912
//
//      SPL3 (length = 1)
//          Offset  Length  Description
//          0       1       Last write to I/O port $1ffd
//          
//
//      RM48 (length = 49152)
//          Offset  Length  Description
//          0       49152   Contents of addresses 16384-65535
//
//      R128 (length = 131072)
//          Offset  Length  Description
//          0       131072  Contents of all banks (0-7)
//
//      EMUL
//          Offset  Length  Description                 Version exists
//          0       2       Version                     0
//          2       2       Number of files opened      0
//          4       2       Number of labels            1
//
//          Data follows:                                                   Version exists
//              Null-terminated strings of the filenames of open files.     0
//              4-byte address, followed by null-terminated label name.     1
//
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <types.h>
#include <config.h>

#include <map>
#include <string>
#include <vector>

class FourCC
{
public:
    FourCC();
    FourCC(u32 fcc);
    FourCC(const FourCC& fcc);
    FourCC(const u8* fcc);

    bool operator==(const FourCC& fcc);
    bool operator!=(const FourCC& fcc);

    bool operator<(const FourCC& fcc) const { return m_fcc < fcc.m_fcc; }

    void write(vector<u8>& data);

private:
    u32 m_fcc;
};

class BlockSection
{
public:
    BlockSection();
    BlockSection(const BlockSection& block);
    BlockSection(FourCC fcc);
    BlockSection(FourCC fcc, const u8* data, u32 size);

    vector<u8>& data()                  { return m_data; }
    const vector<u8>& data() const      { return m_data; }
    FourCC getFcc() const               { return m_fcc; }

    // Used for reading
    u8 peek8(int i) const;
    u16 peek16(int i) const;
    u32 peek32(int i) const;
    i64 peek64(int i) const;
    string peekString(int i) const;
    void peekData(int i, vector<u8>& data, i64 size) const;

    // Used for writing
    void poke8(u8 byte);
    void poke16(u16 word);
    void poke32(u32 dword);
    void poke64(i64 qword);
    void pokeString(const string& s);
    void pokeData(const u8* data, i64 size);
    void pokeData(const vector<u8>& data);
    void checkSize(u32 expectedSize) const;

    void write(vector<u8>& data) const;

private:
    FourCC      m_fcc;
    vector<u8>  m_data;
};

class NxFile
{
public:
    NxFile();

    static vector<u8> loadFile(string fileName);
    static bool saveFile(string fileName, const vector<u8>& data);

    bool load(string fileName);
    bool save(string fileName);

    // Set expectedSize to 0xffffffff (-1) if you don't care.
    void addSection(const BlockSection& section, u32 expectedSize);

    // Queries
    bool hasSection(FourCC fcc);
    u32 sizeSection(FourCC fcc);
    bool checkSection(FourCC fcc, u32 expectedSize);

    const BlockSection& operator[] (FourCC fcc) const;

    // Static data builders
    static u32 read32(const vector<u8>& data, int index);
    static FourCC readFcc(const vector<u8>& data, int index);
    static void write8(vector<u8>& data, u8 x);
    static void write16(vector<u8>& data, u16 x);
    static void write32(vector<u8>& data, u32 x);
    static void writeFcc(vector<u8>& data, FourCC fcc);

private:
    vector<BlockSection> m_sections;
    map<FourCC, int> m_index;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
