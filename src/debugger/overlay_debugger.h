//----------------------------------------------------------------------------------------------------------------------
// Debugger windows
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <editor/editor.h>
#include <asm/asm.h>

#include <string>


//----------------------------------------------------------------------------------------------------------------------
// Memory dump
//----------------------------------------------------------------------------------------------------------------------

class MemoryDumpWindow final : public SelectableWindow
{
public:
    MemoryDumpWindow(Nx& nx);

    void zoomMode(bool flag);

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;
    void onUnselected() override;

    void adjust();
    void poke(u8 value);

private:
    u16     m_address;
    Editor  m_gotoEditor;
    int     m_enableGoto;
    bool    m_showChecksums;

    // Edit mode
    bool    m_editMode;
    u16     m_editAddress;
    int     m_editNibble;
};

//----------------------------------------------------------------------------------------------------------------------
// Disassembler
//----------------------------------------------------------------------------------------------------------------------

class Disassembler;

class DisassemblyWindow final : public SelectableWindow
{
public:
    DisassemblyWindow(Nx& nx);

    void adjustBar();
    void setCursor(u16 address);
    u16 disassemble(Disassembler& d, u16 address);
    u16 disassemble(u16 address);

    void zoomMode(bool flag);

    void setLabels(const vector<pair<string, MemoryMap::Address>>& labels) { m_labels = labels; }
    const Labels& getLabels() const { return m_labels; }

private:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;
    void onUnselected() override;

    u16 backInstruction(u16 address);

    void setView(u16 newTopAddress);
    int findViewAddress(u16 address);
    void cursorDown();
    void cursorUp();

private:
    u16     m_topAddress;
    u16     m_address;

    // This is used to improve cursor movement on disassembly.
    vector<u16> m_viewedAddresses;
    vector<pair<string, MemoryMap::Address>> m_labels;
    int m_firstLabel;
    
    Editor  m_gotoEditor;
    int     m_enableGoto;
};

//----------------------------------------------------------------------------------------------------------------------
// CPU Status
//----------------------------------------------------------------------------------------------------------------------

class Z80;

class CpuStatusWindow final : public Window
{
public:
    CpuStatusWindow(Nx& nx);

private:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

protected:
    Z80&        m_z80;
};

//----------------------------------------------------------------------------------------------------------------------
// Command editor
//----------------------------------------------------------------------------------------------------------------------

class CommandWindow final : public SelectableWindow
{
public:
    CommandWindow(Nx& nx);

    using CommandFunction = function<vector<string> (vector<string>)>;

    void registerCommand(string cmd, CommandFunction handler);

    void zoomMode(bool flag);

private:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

    void onEnter(Editor& editor);

    void prompt();
    string extractInput(EditorData& data);

private:
    Editor m_commandEditor;
    map<string, CommandFunction> m_handlers;
};

//----------------------------------------------------------------------------------------------------------------------
// Debugger
//----------------------------------------------------------------------------------------------------------------------

class Nx;

class Debugger : public Overlay
{
public:
    Debugger(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

    MemoryDumpWindow&   getMemoryDumpWindow() { return m_memoryDumpWindow; }
    DisassemblyWindow&  getDisassemblyWindow() { return m_disassemblyWindow; }
    CpuStatusWindow&    getCpuStatusWindow() { return m_cpuStatusWindow; }
    CommandWindow&      getCommandWindow() { return m_commandWindow; }

private:
    // Command utilities
    vector<string> syntaxCheck(const vector<string>& args, const char* format, vector<string> desc);
    vector<string> describeCommand(const char* format, vector<string> desc);

private:
    MemoryDumpWindow    m_memoryDumpWindow;
    DisassemblyWindow   m_disassemblyWindow;
    CpuStatusWindow     m_cpuStatusWindow;
    CommandWindow       m_commandWindow;

    vector<string>      m_memoryDumpCommands;
    vector<string>      m_disassemblyCommands;
    vector<string>      m_cliCommands;

    bool                m_zoomMode;

    vector<u32>         m_findAddresses;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
