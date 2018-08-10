//----------------------------------------------------------------------------------------------------------------------
//! Implements the Memory, MemAddr & Bank classes.
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <emulator/memory.h>

#include <random>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// B A N K 
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

Bank::Bank(const Bank& other)
    : m_group(other.m_group)
    , m_bank(other.m_bank)
{

}

Bank& Bank::operator= (const Bank& other)
{
    m_group = other.m_group;
    m_bank = other.m_bank;
    return *this;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// M E M A D D R
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

MemAddr::MemAddr(Bank bank, u16 offset)
    : m_bank(bank)
    , m_offset(offset)
{

}

MemAddr::MemAddr(MemGroup group, int realAddress)
    : m_bank(group, u16(realAddress / kBankSize))
    , m_offset(u16(realAddress % kBankSize))
{

}

MemAddr::MemAddr(const MemAddr& memAddr)
    : m_bank(memAddr.m_bank)
    , m_offset(memAddr.m_offset)
{

}

MemAddr& MemAddr::operator= (const MemAddr& memAddr)
{
    m_bank = memAddr.bank();
    m_offset = memAddr.offset();
    return *this;
}

bool MemAddr::operator== (const MemAddr& addr) const
{
    return m_bank == addr.bank() && m_offset == addr.offset();
}

bool MemAddr::operator!= (const MemAddr& addr) const
{
    return m_bank != addr.bank() || m_offset != addr.offset();
}

MemAddr MemAddr::operator+ (int offset) const
{
    int i = index() + offset;
    return MemAddr(bank().group(), i);
}

MemAddr MemAddr::operator- (int offset) const
{
    int i = index() - offset;
    return MemAddr(bank().group(), i);
}

int MemAddr::operator- (MemAddr addr) const
{
    NX_ASSERT(bank().group() == addr.bank().group());
    return index() - addr.index();
}

MemAddr& MemAddr::operator++()
{
    if (++m_offset == kBankSize)
    {
        m_bank = Bank(m_bank.group(), m_bank.index() + 1);
        m_offset = 0;
    }

    return *this;
}

MemAddr MemAddr::operator++(int)
{
    MemAddr m = *this;
    operator++();
    return m;
}

MemAddr& MemAddr::operator--()
{
    if (m_offset-- == 0)
    {
        m_bank = Bank(m_bank.group(), m_bank.index() - 1);
        m_offset = kBankSize - 1;
    }

    return *this;
}

MemAddr MemAddr::operator--(int)
{
    MemAddr m = *this;
    operator--();
    return m;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// M E M O R Y
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

Memory::Memory()
{

}

//----------------------------------------------------------------------------------------------------------------------

void Memory::applySizes(MemorySizes sizes)
{
    for_each(sizes.begin(), sizes.end(), [](int& size) { size *= kBankSize; });
    NX_ASSERT(sizes.size() == m_blocks.size());

    for (size_t i = 0; i < sizes.size(); ++i)
    {
        if (m_blocks[i].size() < (size_t)sizes[i])
        {
            // We need to expand the blocks
            size_t index = m_blocks[i].size();
            m_blocks[i].resize(sizes[i]);

            // Fill up the new memory with random data.
            mt19937 rng;
            rng.seed(random_device()());
            uniform_int_distribution<int> dist(0, 255);
            for (; index < m_blocks[i].size(); ++index)
            {
                m_blocks[i][index] = (u8)dist(rng);
            }
        }
        else
        {
            m_blocks[i].resize(sizes[i]);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Memory::applySlots(const Slots& slots)
{
    m_slots = slots;
}

//----------------------------------------------------------------------------------------------------------------------

bool Memory::isValid(MemAddr addr) const
{
    int index = addr.index();
    return (index > 0) && (index < (int)m_blocks[(int)addr.bank().group()].size());
}

//----------------------------------------------------------------------------------------------------------------------

optional<u16> Memory::convert(MemAddr addr) const
{
    for (int i = 0; i < 8; ++i)
    {
        if (m_slots[i] == addr.bank())
        {
            return (u16)(kBankSize * i + addr.offset());
        }
    }

    return {};
}

//----------------------------------------------------------------------------------------------------------------------

MemAddr Memory::convert(u16 addr) const
{
    int slot = addr / kBankSize;
    int offset = addr % kBankSize;

    return { m_slots[slot], (u16)offset };
}

//----------------------------------------------------------------------------------------------------------------------

u8 Memory::peek8(MemAddr addr) const
{
    return m_blocks[(int)addr.bank().group()][addr.index()];
}

//----------------------------------------------------------------------------------------------------------------------

u8 Memory::peek8(u16 addr) const
{
    return peek8(convert(addr));
}

//----------------------------------------------------------------------------------------------------------------------

void Memory::poke8(MemAddr addr, u8 b)
{
    MemGroup group = addr.bank().group();
    if (group != MemGroup::ROM)
    {
        m_blocks[(int)group][addr.index()] = b;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Memory::poke8(u16 addr, u8 b)
{
    poke8(convert(addr), b);
}

//----------------------------------------------------------------------------------------------------------------------

void Memory::load(Bank bank, const u8* data, int size)
{
    u8* addr = &m_blocks[(int)bank.group()][bank.index() * kBankSize];
    NX_ASSERT(bank.index() * kBankSize + size <= (int)m_blocks[(int)bank.group()].size());
    memcpy(addr, data, (size_t)size);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
