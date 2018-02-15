//----------------------------------------------------------------------------------------------------------------------
// String generation, formatting and parsing routines
//----------------------------------------------------------------------------------------------------------------------

#include <utils/format.h>

string decimalWord(u16 x)
{
    string s;
    int div = 10000;
    while (div > 1 && (x / div == 0)) div /= 10;

    while (div != 0)
    {
        int d = x / div;
        x %= div;
        div /= 10;
        s.push_back('0' + d);
    }

    return s;
}

string decimalByte(u8 x)
{
    return decimalWord(u16(x));
}

string hexWord(u16 x)
{
    string s;
    int div = 4096;

    while (div != 0)
    {
        int d = x / div;
        x %= div;
        div /= 16;
        s.push_back(d < 10 ? '0' + d : 'A' + d - 10);
    }

    return s;
}

string hexByte(u8 x)
{
    return hexWord(u16(x));
}

bool parseNumber(const string& str, int& n)
{
    int base = str[0] == '$' ? 16 : 10;
    int i = str[0] == '$' ? 1 : 0;
    n = 0;

    for (; str[i] != 0; ++i)
    {
        int c = int(str[i]);

        n *= base;
        if (c >= '0' && c <= '9')
        {
            n += (c - '0');
        }
        else if (base == 16 && (c >= 'A' && c <= 'F'))
        {
            n += (c - 'A' + 10);
        }
        else if (base == 16 && (c >= 'a' && c <= 'f'))
        {
            n += (c - 'a' + 10);
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool parseWord(const string& str, u16& out)
{
    int n;
    bool result = parseNumber(str, n) && (n >= 0 && n < 65536);
    if (result) out = u16(n);
    return result;
}

bool parseByte(const string& str, u8& out)
{
    int n;
    bool result = parseNumber(str, n) && (n >= 0 && n < 256);
    if (result) out = u8(n);
    return result;
}

