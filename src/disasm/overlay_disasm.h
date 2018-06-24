//----------------------------------------------------------------------------------------------------------------------
// Disassembly overlay
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <disasm/disassembler.h>
#include <editor/editor.h>
#include <utils/ui.h>

//----------------------------------------------------------------------------------------------------------------------
// Disassembler editor
//----------------------------------------------------------------------------------------------------------------------

class DisassemblerEditor
{
public:
    DisassemblerEditor(Spectrum& speccy, int xCell, int  yCell, int width, int height);

    DisassemblerDoc&        getData()               { return m_data; }
    const DisassemblerDoc&  getData() const         { return m_data; }
    const string&           getFileName() const     { return m_fileName; }
    void                    setFileName(string fn)  { m_fileName = move(fn); }
    string                  getTitle() const;
    DisassemblerDoc::Line&  getCurrentLine()                { return getData().getLine(m_currentLine); }
    int                     getCurrentLineIndex() const     { return m_currentLine; }

    optional<u16>           extractAddress() const          { return getData().extractAddress(m_currentLine); }

    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt);
    void onText(char ch);
    void render(Draw& draw);

    bool isEditing() const { return m_editor != nullptr; }

private:
    void saveFile();
    void ensureVisibleCursor();

    //
    // Use cases
    //
    void insertComment();
    void editComment(bool moveToNextLine);
    void markJump();
    void jump();
    void prevJump();
    void nextJump();
    void clearJumps(int atLine)                 { m_lineNav.clear(); m_lineNav.push_back(atLine); m_navIndex = 0; }

private:
    Spectrum*           m_speccy;
    DisassemblerDoc     m_data;
    int                 m_x;
    int                 m_y;
    int                 m_width;
    int                 m_height;
    int                 m_topLine;          // Line index at top of editor
    int                 m_lineOffset;       // Scroll offset of editor
    int                 m_longestLine;      // Length of longest line rendered
    string              m_fileName;
    Editor*             m_editor;
    bool                m_blockFirstChar;   // Stop the character that initiates the editor to be consumed by it
    string              m_editorPrefix;
    int                 m_currentLine;
    int                 m_nextTag;
    vector<int>         m_lineNav;          // list of lines that we've navigated across
    int                 m_navIndex;

};


//----------------------------------------------------------------------------------------------------------------------
// Disassembler window
//----------------------------------------------------------------------------------------------------------------------

class Nx;

class DisassemblerWindow final : public Window
{
public:
    DisassemblerWindow(Nx& nx);

    DisassemblerEditor& getEditor() { NX_ASSERT(!m_editors.empty()); return m_editors[m_editorOrder[0]]; }
    DisassemblerEditor& getEditor(int i) { NX_ASSERT(i < int(m_editors.size())); return m_editors[m_editorOrder[i]]; }
    int getNumEditors() const { return int(m_editors.size()); }

    bool saveAll();
    bool needToSave() const;

    void openFile(const string& fileName = string());

private:
    void newFile();
    void closeFile();
    void switchTo(const DisassemblerEditor& editor);

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

private:
    vector<DisassemblerEditor>  m_editors;
    vector<int>                 m_editorOrder;
    int                         m_selectedTab;

};

//----------------------------------------------------------------------------------------------------------------------
// Disassembler overlay
//----------------------------------------------------------------------------------------------------------------------

class DisassemblerOverlay final : public Overlay
{
public:
    DisassemblerOverlay(Nx& nx);

    void render(Draw& draw) override;
    void key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void text(char ch) override;
    const vector<string>& commands() const override;

    DisassemblerWindow& getWindow() { return m_window; }

private:
    DisassemblerWindow  m_window;
    vector<string>      m_commands;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
