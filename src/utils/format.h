//------------------------------------------------------------------------------------------------------------------
// String format
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <codecvt>
#include <types.h>
#include <config.h>

//----------------------------------------------------------------------------------------------------------------------
// Character conversion routines
//----------------------------------------------------------------------------------------------------------------------

inline bool isDigit(char c)
{
    return ('0' <= c) && (c <= '9');
}

inline int toDigit(char c)
{
    return c - '0';
}

string decimalWord(u16 x);
string decimalByte(u8 x);
string hexWord(u16 x);
string hexByte(u8 x);

bool parseNumber(const string& str, int& i);
bool parseByte(const string& str, u8& out);
bool parseWord(const string& str, u16& out);

//----------------------------------------------------------------------------------------------------------------------
// Internal code
//----------------------------------------------------------------------------------------------------------------------

namespace {

    template <int N>
    struct Counter
    {
        int m_count[N];
        int m_braceEscapes;
        int m_plainChars;

        Counter() : m_braceEscapes(0), m_plainChars(0)
        {
            memset(m_count, 0, sizeof(int) * N);
        }

        void onMarker(int markerNumber)
        {
            ++m_count[markerNumber];
        }

        void onEscapeLeft() { ++m_braceEscapes; }
        void onEscapeRight() { ++m_braceEscapes; }
        void onChar(char c) { ++m_plainChars; }
    };

    template <int N>
    class Formatter
    {
    public:
        Formatter(string& dest, string* values)
            : m_dest(dest)
            , m_size(0)
            , m_values(values)
        {}

        void onMarker(int markerNumber)
        {
            size_t len = m_values[markerNumber].length();
            m_dest.replace(m_size, len, m_values[markerNumber]);
            m_size += len;
        }

        void onEscapeLeft()
        {
            m_dest[m_size++] = '{';
        }

        void onEscapeRight()
        {
            m_dest[m_size++] = '}';
        }

        void onChar(char c)
        {
            m_dest[m_size++] = c;
        }

    private:
        string & m_dest;
        size_t m_size;
        string* m_values;
    };

    template <int N, typename Handler>
    inline void traverse(const char* format, Handler& callback)
    {
        const char* c = format;
        while (*c != 0)
        {
            if (*c == '{')
            {
                if (c[1] == '{')
                {
                    callback.onEscapeLeft();
                    c += 2;
                }
                else
                {
                    int number = 0;
                    ++c;

                    // Unexpected end of string
                    assert(*c != 0);

                    // Invalid brace contents: must be a positive integer
                    assert(isDigit(*c));

                    number = toDigit(*c++);
                    while (isDigit(*c))
                    {
                        number *= 10;
                        number += toDigit(*c++);
                    }

                    // Unexpected end of string
                    assert(*c != 0);
                    // Invalid brace contents: must be a positive integer
                    assert(*c == '}');
                    // Format value index is out of range
                    assert(number <= N);

                    callback.onMarker(number);
                    ++c;
                } // if (c[1] == '{')
            }
            else if (*c == '}')
            {
                if (c[1] == '}')
                {
                    callback.onEscapeRight();
                    c += 2;
                }
                else
                {
                    // Unescaped right brace
                    assert(0);
                }
            }
            else
            {
                callback.onChar(*c);
                ++c;
            }
        }
    } // traverse

    template <int N>
    inline size_t formattedTotal(string values[N], int counts[N])
    {
        size_t total = 0;
        for (int i = 0; i < N; ++i)
        {
            total += values[i].length() * counts[i];
        }
        return total;
    }

    template <int N>
    inline string formatArray(const char* format, string values[N])
    {
        Counter<N> counter;
        traverse<N>(format, counter);

        size_t formatsSize = formattedTotal<N>(values, counter.m_count);
        size_t outputTotal = formatsSize + counter.m_braceEscapes + counter.m_plainChars;

        string output(outputTotal, 0);
        Formatter<N> formatter(output, values);
        traverse<N>(format, formatter);

        return output;
    }

    template <int N, typename T, typename ...Args>
    inline void internalStringFormat(int index, string values[N], T t, Args... args)
    {
        std::ostringstream ss;
        ss << t;
        values[index] = ss.str();
        internalStringFormat<N>(index + 1, values, args...);
    }

    template <int N>
    inline void internalStringFormat(int index, string values[N])
    {
    }

} // namespace

inline string stringFormat(const char* format)
{
    return string(format);
}

template <typename ...Args>
inline string stringFormat(const char* format, Args... args)
{
    string values[sizeof...(args)];
    internalStringFormat<sizeof...(args), Args...>(0, values, args...);
    return formatArray<sizeof...(args)>(format, values);
}

template <typename... Args>
inline void debugOutput(Args... args)
{
    string s = StringFormat(args...);
    OutputDebugStringA(s.c_str());
}

inline std::string fromWString(const std::wstring& ws)
{
    using conv_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<conv_t, wchar_t> conv;

    return conv.to_bytes(ws);
}

inline std::wstring toWString(const std::string& str)
{
    using conv_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<conv_t, wchar_t> conv;

    return conv.from_bytes(str);
}
