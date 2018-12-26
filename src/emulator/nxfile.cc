//----------------------------------------------------------------------------------------------------------------------
// NxFile implementation
//----------------------------------------------------------------------------------------------------------------------

#include <emulator/nxfile.h>

#include <SFML/System.hpp>

#include <fstream>

//----------------------------------------------------------------------------------------------------------------------
// FourCC
//----------------------------------------------------------------------------------------------------------------------

FourCC::FourCC()
    : m_fcc('0000')
{

}

FourCC::FourCC(u32 fcc)
    : m_fcc(fcc)
{

}

FourCC::FourCC(const FourCC& fcc)
    : m_fcc(fcc.m_fcc)
{

}

FourCC::FourCC(const u8* fcc)
    : m_fcc((fcc[0] << 24) | (fcc[1] << 16) | (fcc[2] << 8) | fcc[3])
{

}

bool FourCC::operator==(const FourCC& fcc)
{
    return m_fcc == fcc.m_fcc;
}

bool FourCC::operator!=(const FourCC& fcc)
{
    return m_fcc != fcc.m_fcc;
}

void FourCC::write(vector<u8>& data)
{
    u32 fcc =
        ((m_fcc & 0xff000000) >> 24) |
        ((m_fcc & 0x00ff0000) >> 8) |
        ((m_fcc & 0x0000ff00) << 8) |
        ((m_fcc & 0x000000ff) << 24);
    NxFile::write32(data, fcc);
}

//----------------------------------------------------------------------------------------------------------------------
// BlockSection
//----------------------------------------------------------------------------------------------------------------------

BlockSection::BlockSection()
    : m_fcc()
    , m_version(0)
    , m_data()
{

}

BlockSection::BlockSection(const BlockSection& block)
    : m_fcc(block.m_fcc)
    , m_version(block.m_version)
    , m_data(block.m_data)
{

}

BlockSection::BlockSection(FourCC fcc, int version)
    : m_fcc(fcc)
    , m_version(version)
    , m_data()
{

}

BlockSection::BlockSection(FourCC fcc, int version, const u8* data, u32 size)
    : m_fcc(fcc)
    , m_version(version)
    , m_data(data, data + size)
{

}

u8 BlockSection::peek8(int i) const
{
    return m_data[i];
}

u16 BlockSection::peek16(int i) const
{
    return u16(m_data[i]) + (u16(m_data[i + 1]) << 8);
}

u32 BlockSection::peek32(int i) const
{
    return u32(peek16(i)) + (u32(peek16(i + 2)) << 16);
}

MemAddr BlockSection::peekAddr(int i) const
{
    return MemAddr(MemGroup::RAM, (int)peek32(i));
}

i64 BlockSection::peek64(int i) const
{
    return i64(peek32(i)) + (i64(peek32(i + 4)) << 32);
}

string BlockSection::peekString(int i) const
{
    const char* start = (const char *)m_data.data() + i;
    const char* end = start;
    while (*end != 0) ++end;
    return string(start, end);
}

void BlockSection::peekData(int i, vector<u8>& data, i64 size) const
{
    data.resize(size);
    copy(m_data.begin() + i, m_data.begin() + i + size, data.begin());
}

void BlockSection::poke8(u8 byte)
{
    m_data.emplace_back(byte);
}

void BlockSection::poke16(u16 word)
{
    m_data.emplace_back(word & 0xff);
    m_data.emplace_back(word >> 8);
}

void BlockSection::poke32(u32 dword)
{
    poke16(dword & 0xffff);
    poke16(dword >> 16);
}

void BlockSection::pokeAddr(MemAddr addr)
{
    u32 a = (u32)addr.index();
    poke32(a);
}

void BlockSection::poke64(i64 qword)
{
    poke32(qword & 0xffffffff);
    poke32(qword >> 32);
}

void BlockSection::pokeString(const string& s)
{
    for (const auto c : s)
    {
        m_data.emplace_back(u8(c));
    }
    m_data.emplace_back(0);
}

void BlockSection::pokeData(const u8* data, i64 size)
{
    copy(data, data + size, back_inserter(m_data));
}

void BlockSection::pokeData(const vector<u8>& data)
{
    copy(data.begin(), data.end(), back_inserter(m_data));
}

void BlockSection::write(vector<u8>& data) const
{
    // Write header
    NxFile::writeFcc(data, m_fcc);
    NxFile::write16(data, u16(m_version));
    NxFile::write32(data, (u32)m_data.size());
    data.insert(data.end(), m_data.begin(), m_data.end());
}

//----------------------------------------------------------------------------------------------------------------------
// NxFile
//----------------------------------------------------------------------------------------------------------------------

NxFile::NxFile()
    : m_sections()
    , m_index()
{

}

vector<u8> NxFile::loadFile(string fileName)
{
    vector<u8> buffer;
    sf::FileInputStream f;

    if (f.open(fileName))
    {
        i64 size = f.getSize();
        buffer.resize(size);
        f.read(buffer.data(), size);
    }

    return buffer;
}

bool NxFile::loadTextFile(string fileName, vector<char>& textBuffer)
{
    sf::FileInputStream f;

    if (f.open(fileName))
    {
        i64 size = f.getSize();
        textBuffer.resize(size);
        f.read(textBuffer.data(), size);
        return true;
    }

    return false;
}

bool NxFile::saveFile(string fileName, const vector<u8>& data)
{
    ofstream f;
    bool result = false;
    f.open(fileName, ios::out | ios::binary | ios::trunc);
    if (f)
    {
        f.write((const char *)data.data(), data.size());
        f.close();
        result = true;
    }

    return result;
}

bool NxFile::saveTextFile(string fileName, const vector<char>& data1, const vector<char>& data2)
{
    ofstream f;
    bool result = false;
    f.open(fileName, ios::out | ios::binary | ios::trunc);
    if (f)
    {
        f.write((const char *)data1.data(), data1.size());
        f.write((const char *)data2.data(), data2.size());
        f.close();
        result = true;
    }

    return result;
}

bool NxFile::load(string fileName)
{
    bool result = false;
    vector<u8> f = loadFile(fileName);
    if (f.size() != 0)
    {
        // Read the header
        FourCC fcc = readFcc(f, 0);
        if (fcc == 'NX00')
        {
            int i = 4;
            while (i < f.size())
            {
                // Read a block
                if ((i + 10) > f.size()) return false;
                FourCC blockFcc = readFcc(f, i);
                i += 4;
                int blockVersion = read16(f, i);
                i += 2;
                u32 blockSize = read32(f, i);
                i += 4;

                if ((i + (int)blockSize) > f.size()) return false;
                m_index[blockFcc] = (int)m_sections.size();
                m_sections.emplace_back(blockFcc, blockVersion, f.data() + i, blockSize);
                i += blockSize;
            }

            result = true;
        }
    }

    return result;
}

bool NxFile::save(string fileName)
{
    vector<u8> data;

    // Write header
    writeFcc(data, 'NX00');

    // Write out the blocks
    for (const BlockSection& block : m_sections)
    {
        block.write(data);
    }

    return saveFile(fileName, data);
}

void NxFile::addSection(const BlockSection& section)
{
    FourCC fcc = section.getFcc();
    assert(!hasSection(fcc));
    m_index[fcc] = (int)m_sections.size();
    m_sections.push_back(section);
}

bool NxFile::hasSection(FourCC fcc)
{
    auto it = m_index.find(fcc);
    return it != m_index.end();
}

u32 NxFile::sizeSection(FourCC fcc)
{
    auto it = m_index.find(fcc);
    if (it == m_index.end())
    {
        return -1;
    }
    else
    {
        return u32(m_sections[it->second].data().size());
    }
}

int NxFile::versionSection(FourCC fcc)
{
    auto it = m_index.find(fcc);
    NX_ASSERT(it != m_index.end());
    return m_sections[it->second].version();
}

bool NxFile::checkSection(FourCC fcc, int version)
{
    return sizeSection(fcc) != -1 && versionSection(fcc) == version;
}

const BlockSection& NxFile::operator[](FourCC fcc) const
{
    auto it = m_index.find(fcc);

    // If this fails, there is no block with this FourCC.
    NX_ASSERT(it != m_index.end());

    return m_sections[it->second];
}

u16 NxFile::read16(const vector<u8>& data, int index)
{
    return (u16(data[index]) |
           (u16(data[index + 1]) << 8));
}

u32 NxFile::read32(const vector<u8>& data, int index)
{
    return (u32(data[index]) |
           (u32(data[index+1]) << 8) |
           (u32(data[index+2]) << 16) |
           (u32(data[index+3]) << 24));
}

FourCC NxFile::readFcc(const vector<u8>& data, int index)
{
    return ((u32(data[index]) << 24) |
            (u32(data[index+1]) << 16) |
            (u32(data[index+2]) << 8) |
            (u32(data[index+3])));
}

void NxFile::write8(vector<u8>& data, u8 x)
{
    data.emplace_back(x);
}

void NxFile::write16(vector<u8>& data, u16 x)
{
    data.emplace_back((x >> 0) & 0xff);
    data.emplace_back((x >> 8) & 0xff);
}

void NxFile::write32(vector<u8>& data, u32 x)
{
    data.emplace_back((x >> 0) & 0xff);
    data.emplace_back((x >> 8) & 0xff);
    data.emplace_back((x >> 16) & 0xff);
    data.emplace_back((x >> 24) & 0xff);
}

void NxFile::writeFcc(vector<u8>& data, FourCC fcc)
{
    fcc.write(data);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
