//----------------------------------------------------------------------------------------------------------------------
// StringTables
// Map a integer-based handle to a string
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>
#include <cstring>
#include <memory>
#include <vector>

#ifdef __APPLE__
#define _stricmp strcasecmp
#define _memicmp strncasecmp
#endif

class StringTable
{
public:
    const int kHashSize = 256;

    StringTable()
    {
        clear();
    }

    i64 addString(const char* str, bool ignoreCase)
    {
        u64 i = hash(str, ignoreCase) % kHashSize;
        size_t hdr = m_hashTable[i];
        while (hdr != 0)
        {
            const char* s = m_strings.data() + m_headers[hdr].m_data;
            if (ignoreCase)
            {
                if (_stricmp(s, (const char *)str) == 0) return i64(hdr);
            }
            else
            {
                if (strcmp(s, (const char *)str) == 0) return i64(hdr);
            }
            hdr = m_headers[hdr].m_next;
        }

        // Not found!
        size_t data = m_strings.size();
        size_t hdrIndex = m_headers.size();
        m_headers.emplace_back(Header{ data, m_hashTable[i] });
        m_hashTable[i] = hdrIndex;
        while (*str != 0)
        {
            char c = *str++;
            c = (ignoreCase && c >= 'a' && c <= 'z') ? c - 32 : c;
            m_strings.emplace_back(c);
        }
        m_strings.emplace_back(0);

        return i64(hdrIndex);
    }

    i64 addRange(const char* start, const char* end, bool ignoreCase)
    {
        u64 i = hash(start, end, ignoreCase) % kHashSize;
        size_t hdr = m_hashTable[i];
        while (hdr != 0)
        {
            const char* s = m_strings.data() + m_headers[hdr].m_data;
            if (ignoreCase)
            {
                // #todo: use case insensititve memcmp
                if (_memicmp(s, start, size_t(end - start)) == 0) return i64(hdr);
            }
            else
            {
                if (memcmp(s, start, size_t(end - start)) == 0) return i64(hdr);
            }
            hdr = m_headers[hdr].m_next;
        }

        // Not found!
        size_t data = m_strings.size();
        size_t hdrIndex = m_headers.size();
        m_headers.emplace_back(Header{ data, size_t(end - start), m_hashTable[i] });
        m_hashTable[i] = hdrIndex;
        while (start != end)
        {
            char c = *start++;
            c = (ignoreCase && c >= 'a' && c <= 'z') ? c - 32 : c;
            m_strings.emplace_back(c);
        }
        m_strings.emplace_back(0);

        return i64(hdrIndex);
    }

    const u8* const get(i64 handle) const
    {
        return (const u8*)(m_strings.data() + m_headers[handle].m_data);
    }

    size_t length(i64 handle) const
    {
        return m_headers[handle].m_size;
    }

    static u64 hash(const char* str, bool ignoreCase)
    {
        u64 h = 14695981039346656037ull;
        while (*str != 0)
        {
            char c = *str++;
            c = (ignoreCase && c >= 'a' && c <= 'z') ? c - 32 : c;
            h ^= c;
            h *= (u64)1099511628211ull;
        }

        return h;
    }

    static u64 hash(const char* start, const char* end, bool ignoreCase)
    {
        u64 h = 14695981039346656037ull;
        while (start != end)
        {
            char c = *start++;
            c = (ignoreCase && c >= 'a' && c <= 'z') ? c - 32 : c;
            h ^= c;
            h *= (u64)1099511628211ull;
        }

        return h;
    }

    void clear()
    {
        m_hashTable.clear();
        m_headers.clear();
        m_strings.clear();

        m_hashTable.resize(kHashSize);
        for (int i = 0; i < kHashSize; ++i) m_hashTable[i] = 0;
        m_headers.emplace_back(Header{ 0, 0 });
        m_strings.emplace_back(0);
    }

private:
    struct Header
    {
        size_t      m_data;     // Index into string blob
        size_t      m_size;     // Length of string
        size_t      m_next;     // Next header with the same hash
    };

    std::vector<size_t>     m_hashTable;        // Fixed size table containing initial offsets into headers table
    std::vector<Header>     m_headers;
    std::vector<char>       m_strings;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

