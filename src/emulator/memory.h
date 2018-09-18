//----------------------------------------------------------------------------------------------------------------------
//! Defines the Memory class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>

#include <array>
#include <optional>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
// Constants

//! The size of a bank (in bytes).
static const int kBankSize = KB(8);

//! Number of bank slots in a memory system.
static const int kNumSlots = 8;

//----------------------------------------------------------------------------------------------------------------------
//! Defines the memory groups in a memory configuration
//!
//! A count of 8K banks are assigned to each memory configuration to create a complete memory system.  For example,
//! the original 48K has 2 ROM banks (16K) and 6 RAM banks (48K) assigned.  These enums define those catgories.

enum class MemGroup
{
    ROM,
    RAM,

    COUNT       // Equal to the number of categories
};

//----------------------------------------------------------------------------------------------------------------------
//! Defines a single 8K bank
//!
//! Also note that order of banks is undefined in physical memory.  Banks need to be converted to their Z80 address
//! (i.e. 64K address range) before order can be decided.  Although there is order with bytes within a bank, the order
//! us undefined across bank borders and indeed across memory category borders.

class Bank
{
public:
    Bank() : m_group(MemGroup::RAM), m_bank(0) {}
    Bank(MemGroup group, u16 bank) : m_group(group), m_bank(bank) {}
    Bank(const Bank& other);
    Bank& operator= (const Bank& other);

    //! Returns the memory group that this bank resides in.
    MemGroup group() const { return m_group; }

    //! Returns the index into the memory group that this bank resides in.
    u16 index() const { return m_bank; }

    //! Compares one bank to another.
    bool operator== (const Bank& bank) const { return m_group == bank.m_group && m_bank == bank.m_bank; }

    //! Compares one bank to another.
    bool operator!= (const Bank& bank) const { return m_group != bank.m_group || m_bank != bank.m_bank; }

private:
    MemGroup    m_group;
    u16         m_bank;
};

//----------------------------------------------------------------------------------------------------------------------
//! Defines an address in physical memory

class MemAddr
{
public:
    // Default points to first RAM address.
    MemAddr() : m_bank(), m_offset(0) {}

    MemAddr(Bank bank, u16 offset);
    MemAddr(MemGroup group, int realAddress);

    MemAddr(const MemAddr& memAddr);
    MemAddr& operator= (const MemAddr& memAddr);

    //! Returns the bank this memory address resides in.
    Bank bank() const { return m_bank; }

    //! Returns the offset into the bank this memory address resides in.
    u16 offset() const { return m_offset; }

    //! Returns an index into the memory block that this address resides in.
    int index() const { return m_bank.index() * kBankSize + m_offset; }

    bool operator== (const MemAddr& addr) const;
    bool operator!= (const MemAddr& addr) const;

    //! Add an offset (in bytes) to an address.
    //!
    //! If this causes the address to go into the next bank, the bank number will be advanced.
    //!
    //! @param  offset  The offset in bytes.
    //! @returns        The resulting address after advancing 'offset' bytes.
    //!
    //! @note   This could cause an invalid overflow if the bank number is greater than the current memory system
    //!         allows.  To handle this use Memory::isValid() to check it.
    MemAddr operator+ (int offset) const;

    //! Subtract an offset (in bytes) to an address.
    //!
    //! If this causes the address to go into the previous bank, the bank number will be decremented.
    //!
    //! @param  offset  The offset in bytes.
    //! @returns        The resulting address after retreating 'offset' bytes.
    //!
    //! @note   This could cause an invalid overflow if the bank number is less than 0.  To handle this use 
    //!         Memory::isValid() to check it.
    MemAddr operator- (int offset) const;

    //! Find the difference (in bytes) between two addresses.
    //!
    //! If this address is before the given address, the difference will be negative.
    //!
    //! @param  addr    The second address.
    //! @returns        Returns the result of the second address subtracted from this address.
    //!
    int operator- (MemAddr addr) const;

    //! Pre-increment the address.
    //!
    //! @note       This can cause an overflow into the next bank, which might result in an invalid address.  Use
    //!             Memory::isValid() to check this.
    MemAddr& operator++();

    //! Post-increment the address.
    //!
    //! @note       This can cause an overflow into the next bank, which might result in an invalid address.  Use
    //!             Memory::isValid() to check this.
    MemAddr operator++(int);

    //! Pre-decrement the address.
    //!
    //! @note       This can cause an overflow into the previous bank, which might result in an invalid address.  Use
    //!             Memory::isValid() to check this.
    MemAddr& operator--();

    //! Post-decrement the address.
    //!
    //! @note       This can cause an overflow into the previous bank, which might result in an invalid address.  Use
    //!             Memory::isValid() to check this.
    MemAddr operator--(int);

private:
    Bank        m_bank;     //!< The bank number.
    u16         m_offset;   //!< The offset (in bytes) in the bank.
};

//----------------------------------------------------------------------------------------------------------------------
//! Defines the memory configuration
//!
//! The memory system has 8 8K slots to which each a bank can be assigned to.  Banks are allocated in memory
//! category blocks (such as ROM, RAM, DivIDE etc).  Each slot is then assigned to a bank within one of those blocks.

class Memory
{
public:
    Memory();

    //! A block of memory.
    //!
    //! This block of memory will be divided up into 8K banks, so the size should always be a multiple of 8K.
    //! Also a block represents one type of memory that will be placed into a memory category (e.g. ROM, RAM etc.).
    using Block = vector<u8>;

    //! All the memory in a system
    //!
    //! A memory system consists of memory blocks for each memory group.
    using MemorySystem = array<Block, (int)MemGroup::COUNT>;

    //! Size configuration for the system
    //!
    //! Each element defines the number of banks per memory group.
    using MemorySizes = array<int, (int)MemGroup::COUNT>;

    //! Represents a bank configuration for the virtual memory.
    //!
    //! The Z80 CPU can only see 64K memory (virtual memory).  So the virtual memory needs to be mapped on to
    //! physical memory.  The slots do this.  Each slot represents 8K of physical memory.
    using Slots = array<Bank, 8>;

    //! Applies new sizes to the various memory categories.  
    //!
    //! This will truncate or extend the memory blocks accordingly.  If the memory is extended, random noise is
    //! written to the memory.
    void applySizes(MemorySizes sizes);

    //! Apply the slots configuration.
    void applySlots(const Slots& slots);

    //! Validates an address.
    bool isValid(MemAddr addr) const;

    //! Convert memory address from physical to virtual.
    optional<u16> convert(MemAddr addr) const;

    //! Convert memory address from virtual to physical.
    MemAddr convert(u16 addr) const;

    //! Returns true if the Z80 can see this address, i.e. it's in one of the slots.
    bool isZ80Address(MemAddr addr) const;

    //! Read a byte from physical memory.
    u8 peek8(MemAddr addr) const;

    //! Read a byte from virtual memory.
    u8 peek8(u16 addr) const;

    //! Write a byte to physical memory.
    void poke8(MemAddr addr, u8 b);

    //! Write a byte to virtual memory.
    void poke8(u16 addr, u8 b);

    //! Load data into consecutive banks.
    void load(Bank bank, const u8* data, int size);

private:
    MemorySystem    m_blocks;
    Slots           m_slots;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

