//----------------------------------------------------------------------------------------------------------------------
// Filename utilities
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <config.h>
#include <types.h>

string fullFileName(string originalFileName, string newFileName);



//----------------------------------------------------------------------------------------------------------------------
// Path class
//----------------------------------------------------------------------------------------------------------------------

class Path
{
public:
    Path();
    Path(string osPath);

    bool empty() const { return m_root.empty() && m_elems.size() == 0; }
    const string& root() const { return m_root; }
    const vector<string>& elems() const { return m_elems; }
    const string& operator[] (int i) const { return m_elems[i]; }

    vector<string>::const_iterator cbegin() const { return m_elems.cbegin(); }
    vector<string>::const_iterator cend() const { return m_elems.cend(); }

    bool valid() const;
    bool isRelative() const { return m_isRelative; }
    bool exists() const;
    bool hasExtension() const;
    string extension() const;

    Path operator / (const Path& path);
    Path operator / (const string& path);

    string osPath() const;
    Path parent() const;

private:
    void setPath(string osPath);

private:
    string          m_root;         // On Windows, it's the drive letter followed by ':'
    vector<string>  m_elems;        // Elements of the path
    bool            m_isRelative;   // True, if the original path wasn't full
};
