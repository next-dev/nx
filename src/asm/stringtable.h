//----------------------------------------------------------------------------------------------------------------------
// StringTables
// Map a integer-based handle to a string
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <types.h>

class StringTable
{
public:
    const int kHashSize = 256;

    StringTable()
    {
        m_hashTable.resize(kHashSize);
        for (int i = 0; i < kHashSize; ++i) m_hashTable[i] = 0;
        m_headers.emplace_back(Header{ 0, 0 });
        m_strings.emplace_back(0);
    }

    i64 add(const char* str)
    {
        u64 i = hash(str) % kHashSize;
        size_t hdr = m_hashTable[i];
        while (hdr != 0)
        {
            const char* s = m_strings.data() + m_headers[hdr].m_data;
            if (strcmp(s, str) == 0) return i64(hdr);
            hdr = m_headers[hdr].m_next;
        }

        // Not found!
        size_t data = m_strings.size();
        size_t hdrIndex = m_headers.size();
        m_headers.emplace_back(Header{ data, m_hashTable[i] });
        m_hashTable[i] = hdrIndex;
        while (*str != 0)
        {
            m_strings.emplace_back(*str++);
        }
        m_strings.emplace_back(0);

        return i64(hdrIndex);
    }

    const char* const get(i64 handle) const
    {
        return m_strings.data() + m_headers[handle].m_data;
    }

private:
    u64 hash(const char* str)
    {
        u64 h = 14695981039346656037;
        while (*str != 0)
        {
            h ^= *str++;
            h *= (u64)1099511628211ull;
        }

        return h;
    }

private:
    struct Header
    {
        size_t      m_data;
        size_t      m_next;
    };

    std::vector<size_t>     m_hashTable;        // Fixed size table containing initial offsets into headers table
    std::vector<Header>     m_headers;
    std::vector<char>       m_strings;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

