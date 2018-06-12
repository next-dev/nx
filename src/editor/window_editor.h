//----------------------------------------------------------------------------------------------------------------------
// Editor in a window
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <types.h>
#include <config.h>

#include <utils/ui.h>

//----------------------------------------------------------------------------------------------------------------------
// EditorWindow
//----------------------------------------------------------------------------------------------------------------------

class EditorWindow final : public Window
{
public:
    EditorWindow(Nx& nx, string title);

    Editor& getEditor() { NX_ASSERT(!m_editors.empty()); return m_editors[m_editorOrder[0]]; }
    Editor& getEditor(int i) { NX_ASSERT(i < int(m_editors.size())); return m_editors[m_editorOrder[i]]; }
    int getNumEditors() const { return int(m_editors.size()); }

    bool saveAll();
    bool hasData() const { return m_editors.size() > 0; }
    bool needToSave() const;

    void openFile(const string& fileName = string());

    void setErrorInfos(vector<ErrorInfo> errors);
    void goToError(int n);

private:
    void newFile();
    void closeFile();
    void switchTo(const Editor& editor);
    void setStatus(string str, u8 colour) { m_status = str; m_statusColour = colour; }
    void setDefaultStatus();

protected:
    void onDraw(Draw& draw) override;
    void onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt) override;
    void onText(char ch) override;

private:
    vector<Editor>                  m_editors;
    vector<int>                     m_editorOrder;
    vector<ErrorInfo>               m_errors;
    int                             m_currentError;

    int                             m_selectedTab;      // Current tab index during selection window
    string                          m_status;
    u8                              m_statusColour;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------


