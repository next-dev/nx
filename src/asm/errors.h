//----------------------------------------------------------------------------------------------------------------------
// Error manager
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>

#include <asm/lex.h>

//----------------------------------------------------------------------------------------------------------------------
// ErrorInfo
// Stores information about an error
//----------------------------------------------------------------------------------------------------------------------

struct ErrorInfo
{
    string      m_fileName;
    string      m_error;
    int         m_line;
    int         m_column;

    ErrorInfo(string fileName, string error, int line, int column)
        : m_fileName(fileName)
        , m_error(error)
        , m_line(line)
        , m_column(column)
    {}

    ErrorInfo() {}
};

//----------------------------------------------------------------------------------------------------------------------
// ErrorManager
// Manages error meta-information and output
//----------------------------------------------------------------------------------------------------------------------

class ErrorManager
{
public:
    void error(const Lex& l, const Lex::Element& el, const string& message);
    void error(const string& fileName, const string& message, int line, int column);
    void output(const string& message) { m_output.emplace_back(message); }

    const vector<string>& getOutput() const { return m_output; }
    const vector<ErrorInfo>& getErrors() const { return m_errors; }

    void clearOutput() { m_output.clear(); }
    void reset() { clearOutput(); m_errors.clear(); }

private:
    vector<string> m_output;
    vector<ErrorInfo> m_errors;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
