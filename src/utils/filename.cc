//----------------------------------------------------------------------------------------------------------------------
// Filename utilities
//----------------------------------------------------------------------------------------------------------------------

#include <utils/filename.h>

Path fullFileName(Path originalFileName, Path newFileName)
{
    // If originalFileName starts with a '<', then it isn't interpreted as an actual
    // filename.  So just return newFileName.
    if (!originalFileName.valid()) return newFileName;

    // If the new filename doesn't start with a "/" or an "\" then it is relative.
    // Find the path of the original filename and append the newFileName
    if (newFileName.isRelative())
    {
        return originalFileName.parent() / newFileName;
    }

    return newFileName;
}


//----------------------------------------------------------------------------------------------------------------------
// Path class
//----------------------------------------------------------------------------------------------------------------------

Path::Path()
{
    setPath("/");
}

Path::Path(string osPath)
    : m_isRelative(false)
{
    setPath(osPath);
}

void Path::setPath(string osPath)
{
    m_root = "";

#ifdef _WIN32
    if (osPath.size() > 2 && isalpha(osPath[0]) && osPath[1] == ':')
    {
        m_root = string(osPath.begin(), osPath.begin() + 2);
        osPath = osPath.substr(2);
    }
#endif

    m_isRelative = !(osPath.size() > 1 && (osPath[0] == '/' || osPath[0] == '\\'));
    if (!m_isRelative) osPath = osPath.substr(1);

    string elem;
    for (auto c : osPath)
    {
        if (((c == '/') || (c == '\\')) && (elem.size() > 0))
        {
            if (elem.size() > 0) m_elems.push_back(elem);
            elem.clear();
        }
        else
        {
            elem += c;
        }
    }
    if (elem.size() != 0) m_elems.push_back(elem);
}

bool Path::valid() const
{
    const string reservedChars = "<>:\"/\\|?*";

    if (m_root.empty() && m_elems.empty()) return false;
    for (const auto& elem : m_elems)
    {
        for (char rc : reservedChars)
        {
            if (elem.find(rc) != elem.npos) return false;
        }
    }

    return true;
}

Path Path::operator / (const Path& path)
{
    assert(path.isRelative());
    Path p = *this;

    for (const auto& elem : path.elems())
    {
        p.m_elems.push_back(elem);
    }

    return p;
}

Path Path::operator / (const string& str)
{
    return operator /(Path(str));
}

string Path::osPath() const
{
#ifdef _WIN32
    string p = m_root;
    if (!isRelative()) p += '\\';

    for (const auto& elem : m_elems)
    {
        p += elem + '\\';
    }

#else
    // POSIX
    string p;
    if (!isRelative()) p += '/';

    for (const auto& elem : m_elems)
    {
        p += elem + '/';
    }
#endif

    p.erase(p.end() - 1);
    return p;
}

Path Path::parent() const
{
    Path p = *this;
    assert(p.m_elems.size() != 0);
    p.m_elems.erase(p.m_elems.end() - 1);
    return p;
}

bool Path::hasExtension() const
{
    if (m_elems.size() == 0) return false;
    return m_elems.back().find('.') != 	string::npos;
}

string Path::extension() const
{
    assert(m_elems.size() > 0);
    size_t i = m_elems.back().find('.');
    return m_elems.back().substr(i);
}
