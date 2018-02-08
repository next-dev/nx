//------------------------------------------------------------------------------------------------------------------
// String format
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <codecvt>


//----------------------------------------------------------------------------------------------------------------------
// Character conversion routines
//----------------------------------------------------------------------------------------------------------------------

inline bool IsDigit(char c)
{
    return ('0' <= c) && (c <= '9');
}

inline int ToDigit(char c)
{
    return c - '0';
}

//----------------------------------------------------------------------------------------------------------------------
// Internal code
//----------------------------------------------------------------------------------------------------------------------

namespace {

    class FormatStringException : public std::runtime_error
    {
    public:
        FormatStringException(const char* msg) : std::runtime_error(msg) {}
    };

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

        void OnMarker(int markerNumber)
        {
            ++m_count[markerNumber];
        }

        void OnEscapeLeft() { ++m_braceEscapes; }
        void OnEscapeRight() { ++m_braceEscapes; }
        void OnChar(char c) { ++m_plainChars; }
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

        void OnMarker(int markerNumber)
        {
            size_t len = m_values[markerNumber].length();
            m_dest.replace(m_size, len, m_values[markerNumber]);
            m_size += len;
        }

        void OnEscapeLeft()
        {
            m_dest[m_size++] = '{';
        }

        void OnEscapeRight()
        {
            m_dest[m_size++] = '}';
        }

        void OnChar(char c)
        {
            m_dest[m_size++] = c;
        }

    private:
        string & m_dest;
        size_t m_size;
        string* m_values;
    };

    template <int N, typename Handler>
    inline void Traverse(const char* format, Handler& callback)
    {
        const char* c = format;
        while (*c != 0)
        {
            if (*c == '{')
            {
                if (c[1] == '{')
                {
                    callback.OnEscapeLeft();
                    c += 2;
                }
                else
                {
                    int number = 0;
                    ++c;

                    if (*c == 0) { throw FormatStringException("Unexpected end of string"); }

                    if (!IsDigit(*c)) { throw FormatStringException("Invalid brace contents: must be a positive integer"); }

                    number = ToDigit(*c++);
                    while (IsDigit(*c))
                    {
                        number *= 10;
                        number += ToDigit(*c++);
                    }

                    if (*c == 0) { throw FormatStringException("Unexpected end of string"); }
                    if (*c != '}') { throw FormatStringException("Invalid brace contents: must be a positive integer"); }

                    if (number > N) { throw FormatStringException("Format value index is out of range"); }

                    callback.OnMarker(number);
                    ++c;
                } // if (c[1] == '{')
            }
            else if (*c == '}')
            {
                if (c[1] == '}')
                {
                    callback.OnEscapeRight();
                    c += 2;
                }
                else
                {
                    throw FormatStringException("Un-escaped right brace");
                }
            }
            else
            {
                callback.OnChar(*c);
                ++c;
            }
        }
    } // traverse

    template <int N>
    inline size_t FormattedTotal(string values[N], int counts[N])
    {
        size_t total = 0;
        for (int i = 0; i < N; ++i)
        {
            total += values[i].length() * counts[i];
        }
        return total;
    }

    template <int N>
    inline string FormatArray(const char* format, string values[N])
    {
        Counter<N> counter;
        Traverse<N>(format, counter);

        size_t formatsSize = FormattedTotal<N>(values, counter.m_count);
        size_t outputTotal = formatsSize + counter.m_braceEscapes + counter.m_plainChars;

        string output(outputTotal, 0);
        Formatter<N> formatter(output, values);
        Traverse<N>(format, formatter);

        return output;
    }

    template <int N, typename T, typename ...Args>
    inline void InternalStringFormat(int index, string values[N], T t, Args... args)
    {
        std::ostringstream ss;
        ss << t;
        values[index] = ss.str();
        InternalStringFormat<N>(index + 1, values, args...);
    }

    template <int N>
    inline void InternalStringFormat(int index, string values[N])
    {
    }

} // namespace

inline string StringFormat(const char* format)
{
    return string(format);
}

template <typename ...Args>
inline string StringFormat(const char* format, Args... args)
{
    string values[sizeof...(args)];
    InternalStringFormat<sizeof...(args), Args...>(0, values, args...);
    return FormatArray<sizeof...(args)>(format, values);
}

template <typename... Args>
inline void DebugOutput(Args... args)
{
    string s = StringFormat(args...);
    OutputDebugStringA(s.c_str());
}

inline std::string FromWString(const std::wstring& ws)
{
    using conv_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<conv_t, wchar_t> conv;

    return conv.to_bytes(ws);
}

inline std::wstring ToWString(const std::string& str)
{
    using conv_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<conv_t, wchar_t> conv;

    return conv.from_bytes(str);
}
