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
//      SN48 (length = 40)
//          Offset  Length  Description
//          0       2       Contents of AF
//          2       2       Contents of BC
//          4       2       Contents of DE
//          8       2       Contents of HL
//          10      2       Contents of AF'
//          12      2       Contents of BC'
//          14      2       Contents of DE'
//          18      2       Contents of HL'
//          20      2       Contents of IX
//          22      2       Contents of IY
//          24      2       Contents of SP
//          26      2       Contents of PC
//          28      2       Contents of IR
//          30      2       Contents of WZ
//          32      1       Interrupt mode
//          33      1       IFF1
//          34      1       IFF2
//          35      1       Border colour
//          36      4       T-state
//
//      RM48 (length = 49152)
//          Offset  Length  Description
//          0       49152   Contents of addresses 16384-65535
//          
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "types.h"
#include "config.h"

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

    // Used for writing
    void poke8(u8 byte);
    void poke16(u16 word);
    void poke32(u32 dword);
    void checkSize(u32 expectedSize);

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
    static void write32(vector<u8>& data, u32 x);
    static void writeFcc(vector<u8>& data, FourCC fcc);

private:
    vector<BlockSection> m_sections;
    map<FourCC, int> m_index;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
