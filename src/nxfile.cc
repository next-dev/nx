//----------------------------------------------------------------------------------------------------------------------
// NxFile implementation
//----------------------------------------------------------------------------------------------------------------------

#include "nxfile.h"
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
        ((m_fcc & 0x00ff0000) >> 16) |
        ((m_fcc & 0x0000ff00) >> 8) |
        ((m_fcc & 0x000000ff));
    NxFile::write32(data, fcc);
}

//----------------------------------------------------------------------------------------------------------------------
// BlockSection
//----------------------------------------------------------------------------------------------------------------------

BlockSection::BlockSection()
    : m_fcc()
    , m_data()
{

}

BlockSection::BlockSection(const BlockSection& block)
    : m_fcc(block.m_fcc)
    , m_data(block.m_data)
{

}

BlockSection::BlockSection(FourCC fcc)
    : m_fcc(fcc)
    , m_data()
{

}

BlockSection::BlockSection(FourCC fcc, const u8* data, u32 size)
    : m_fcc(fcc)
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

void BlockSection::checkSize(u32 expectedSize)
{
    NX_ASSERT(m_data.size() == (size_t)expectedSize);
}

void BlockSection::write(vector<u8>& data) const
{
    // Write header
    NxFile::writeFcc(data, m_fcc);
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
                if ((i + 8) > f.size()) return false;
                FourCC blockFcc = readFcc(f, i);
                i += 4;
                u32 blockSize = read32(f, i);
                i += 4;

                if ((i + (int)blockSize) > f.size()) return false;
                m_index[blockFcc] = (int)m_sections.size();
                m_sections.emplace_back(blockFcc, f.data() + i, blockSize);
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

void NxFile::addSection(const BlockSection& section, u32 expectedSize)
{
    FourCC fcc = section.getFcc();
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

bool NxFile::checkSection(FourCC fcc, u32 expectedSize)
{
    bool check = sizeSection(fcc) == expectedSize;
    NX_ASSERT(check);
    return check;
}

const BlockSection& NxFile::operator[](FourCC fcc) const
{
    auto it = m_index.find(fcc);

    // If this fails, there is no block with this FourCC.
    NX_ASSERT(it != m_index.end());

    return m_sections[it->second];
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
