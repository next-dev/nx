//----------------------------------------------------------------------------------------------------------------------
// Window Editor
//----------------------------------------------------------------------------------------------------------------------

#include <editor/window_editor.h>
#include <utils/format.h>
#include <utils/tinyfiledialogs.h>

//----------------------------------------------------------------------------------------------------------------------
// EditorWindow
//----------------------------------------------------------------------------------------------------------------------

EditorWindow::EditorWindow(Nx& nx, string title)
    : Window(nx, 1, 1, 78, 59, title, Colour::Blue, Colour::Black, false)
    , m_editors()
    , m_selectedTab(-1)
    , m_status("Line: {6l}, Column: {6c}")
    , m_statusColour(Draw::attr(Colour::White, Colour::Blue, true))
    , m_currentError(-1)
{
}

void EditorWindow::setDefaultStatus()
{
    setStatus("Line: {6l}, Column: {6c}", Draw::attr(Colour::White, Colour::Blue, true));
}

void EditorWindow::onDraw(Draw& draw)
{
    if (m_editors.empty())
    {
        string line1 = "Press {Ctrl-O} to open a file for editing";
        string line2 = "Press {Ctrl-N} to create a new file";
        u8 colour = draw.attr(Colour::White, Colour::Black, false);

        int y = m_y + (m_height / 2);

        draw.printString(m_x + (m_width - int(line1.size())) / 2, y - 1, line1, true, colour);
        draw.printString(m_x + (m_width - int(line2.size())) / 2, y + 1, line2, true, colour);
    }
    else
    {
        getEditor().renderAll(draw);

        //
        // If Ctrl+Tab is pressed draw the menu
        //
        if (m_selectedTab >= 0)
        {
            int maxWidth = 0;
            for (size_t i = 0; i < m_editorOrder.size(); ++i)
            {
                Editor& editor = m_editors[m_editorOrder[i]];
                int width = draw.squashedStringWidth(editor.getTitle());
                maxWidth = std::max(width + 2, maxWidth);
            }

            maxWidth = std::max(20, maxWidth);
            draw.window(m_x + 1, m_y + 1, maxWidth + 2, int(m_editorOrder.size() + 2), "Buffers", true);

            for (size_t i = 0; i < m_editorOrder.size(); ++i)
            {
                u8 colour = i == m_selectedTab ? Draw::attr(Colour::White, Colour::Red, true) : Draw::attr(Colour::Black, Colour::White, true);
                Editor& editor = m_editors[m_editorOrder[i]];
                for (int x = 0; x < maxWidth; ++x)
                {
                    draw.printChar(m_x + 2 + x, m_y + 2 + int(i), ' ', colour);
                }
                draw.printSquashedString(m_x + 2, m_y + 2 + int(i), editor.getTitle(), colour);
            }
        }
    }

    // Draw status bar
    draw.attrRect(m_x + 0, m_y + m_height, m_width, 1, m_statusColour);

    if (!m_editors.empty())
    {
        string line;
        for (const char* str = m_status.data(); *str != 0; ++str)
        {
            if (*str == '{')
            {
                // Special formatting
                int pad = 0;
                ++str;
                while (*str != '}')
                {
                    if (*str >= '0' && *str <= '9')
                    {
                        pad = (pad * 10) + (*str - '0');
                    }
                    else switch (*str)
                    {
                    case 'l':
                        // Line number
                    {
                        string x = intString(getEditor().getData().getCurrentLine() + 1, pad);
                        line += x;
                        pad = 0;
                    }
                    break;

                    case 'c':
                        // Column number
                    {
                        string x = intString(getEditor().getData().getCurrentPosInLine() + 1, pad);
                        line += x;
                        pad = 0;
                    }
                    break;

                    default:
                        break;
                    }
                    ++str;
                }
            }
            else
            {
                line += *str;
            }
        }
        draw.printSquashedString(m_x + 1, m_y + m_height, line, m_statusColour);
    }
}

void EditorWindow::newFile()
{
    int index = int(m_editors.size());
    m_editors.emplace_back(2, 2, 76, 57, Draw::attr(Colour::White, Colour::Black, false), false, 1024, 1024);
    m_editors.back().setCommentColour(Draw::attr(Colour::Green, Colour::Black, false));
    m_editors.back().setLineNumberColour(Draw::attr(Colour::Cyan, Colour::Black, false));
    m_editors.back().getData().setTabs({ 8, 14, 32 }, 4);
    m_editorOrder.insert(m_editorOrder.begin(), index);

#if NX_DEBUG_CONSOLE
    cout << "NEW -------------------------------\n\n";
    for (const auto& editor : m_editors)
    {
        cout << (editor.getFileName().empty() ? "[new file]" : editor.getFileName()) << endl;
    }
    cout << endl;
#endif
}

void EditorWindow::closeFile()
{
    if (m_editors.empty()) return;

    if (getEditor().getData().hasChanged())
    {
        // Check to see if the user really wants to overwrite their changes.
        if (!tinyfd_messageBox("Are you sure?", "There has been changes since you last saved.  Are you sure you want to lose your changes?",
            "yesno", "question", 0))
        {
            return;
        }
    }

    int index = m_editorOrder[0];
    m_editors.erase(m_editors.begin() + index);
    m_editorOrder.erase(m_editorOrder.begin());
    for (auto& order : m_editorOrder)
    {
        if (order > index) order -= 1;
    }

#if NX_DEBUG_CONSOLE
    cout << "NEW -------------------------------\n\n";
    for (const auto& editor : m_editors)
    {
        cout << (editor.getFileName().empty() ? "[new file]" : editor.getFileName()) << endl;
    }
    cout << endl;
#endif
}

void EditorWindow::openFile(const string& fileName /* = string() */)
{
    const char* fn = 0;
    if (fileName.empty())
    {
        const char* filters[] = { "*.asm", "*.s" };
        fn = tinyfd_openFileDialog("Load source code", 0, sizeof(filters) / sizeof(filters[0]),
            filters, "Source code", 0);
    }
    else
    {
        fn = fileName.c_str();
    }

    if (fn)
    {
        // Make sure we don't already have it open.
        for (int i = 0; i < getNumEditors(); ++i)
        {
            if (getEditor(i).getFileName() == fn)
            {
                switchTo(getEditor(i));
                return;
            }
        }

        newFile();

        Editor& thisEditor = getEditor();
        if (thisEditor.getData().load(fn))
        {
            thisEditor.setFileName(fn);
        }
        else
        {
            string msg = stringFormat("Unable to open file '{0}'.", fileName);
            tinyfd_messageBox("ERROR", msg.c_str(), "ok", "warning", 0);
            closeFile();
        }
    }

#if NX_DEBUG_CONSOLE
    cout << "NEW -------------------------------\n\n";
    for (const auto& editor : m_editors)
    {
        cout << (editor.getFileName().empty() ? "[new file]" : editor.getFileName()) << endl;
    }
    cout << endl;
#endif
}

void EditorWindow::switchTo(const Editor& editor)
{
    int editorIndex = int(&editor - m_editors.data());
    int tabIndex = int(find(m_editorOrder.begin(), m_editorOrder.end(), editorIndex) - m_editorOrder.begin());
    m_editorOrder.erase(m_editorOrder.begin() + tabIndex);
    m_editorOrder.insert(m_editorOrder.begin(), editorIndex);
}

void EditorWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    bool resetStatus = true;

    if (key == K::LShift || key == K::RShift || key == K::LAlt || key == K::RAlt ||
        key == K::LControl || key == K::RControl || key == K::Unknown)
    {
        resetStatus = false;
    }

    if (down && ctrl && !shift && !alt)
    {
        switch (key)
        {
        case K::N:  // New file
            newFile();
            break;

        case K::W:  // Close file
            closeFile();
            break;

        case K::O:  // Open file
            openFile();
            break;
                
        default:
            break;
        }
    }

    if (!ctrl && !alt && key == K::F4)
    {
        if (down)
        {
            if (shift)
            {
                if (--m_currentError < 0) m_currentError = max(0, (int)m_errors.size() - 1);
            }
            else
            {
                if (++m_currentError >= m_errors.size()) m_currentError = 0;
            }
            goToError(m_currentError);
        }
        resetStatus = false;
    }

    if (!m_editors.empty())
    {
        if (m_selectedTab == -1) getEditor().key(key, down, shift, ctrl, alt);

        if (down && ctrl && !alt && !shift)
        {
            if (key == K::Tab)
            {
                if (m_selectedTab == -1)
                {
                    // First time selecting tab
                    m_selectedTab = 1;
                }
                else
                {
                    ++m_selectedTab;
                }
                if (m_selectedTab >= m_editors.size()) m_selectedTab = 0;
            }
        }

        if ((m_selectedTab >= 0) && !down && !ctrl && !shift && !alt)
        {
            int index = m_editorOrder[m_selectedTab];
            // #todo: use switchTo
            m_editorOrder.erase(m_editorOrder.begin() + m_selectedTab);
            m_editorOrder.insert(m_editorOrder.begin(), index);
            m_selectedTab = -1;
        }

        // Set the title
        string title = getEditor().getTitle();
        setTitle(string("Editor/Assembler - ") + title);
    }
    else
    {
        setTitle(string("Editor/Assembler"));
    }

    if (resetStatus)
    {
        setDefaultStatus();
    }
}

void EditorWindow::onText(char ch)
{
    if (!m_editors.empty())
    {
        getEditor().text(ch);
    }
    setDefaultStatus();
}

bool EditorWindow::saveAll()
{
    bool asked = false;
    bool saveUnnamedFiles = false;

    if (m_editors.empty()) return true;

    for (auto& editor : m_editors)
    {
        if (!editor.getData().hasChanged()) continue;

        const string& fileName = editor.getFileName();

        if (!asked && fileName.empty())
        {
            int result = tinyfd_messageBox(
                "Unsaved files detected",
                "There are some new files open in the editor that are unsaved.  Do you still wish to save "
                "these files before continuing?",
                "yesnocancel", "question", 0);
            switch (result)
            {
            case 0:     // Cancel - stop everything!
                return false;
            case 1:     // Yes - trigger save of unnamed/unsaved files
                asked = true;
                saveUnnamedFiles = true;
                break;
            case 2:     // No - do not save
                asked = true;
                saveUnnamedFiles = false;
                break;
            }
        }

        if (!fileName.empty() || saveUnnamedFiles)
        {
            editor.save(editor.getFileName());
        }
    }

    return true;
}

bool EditorWindow::needToSave() const
{
    for (auto& editor : m_editors)
    {
        if (editor.getData().hasChanged()) return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// Error cycling
//----------------------------------------------------------------------------------------------------------------------

void EditorWindow::setErrorInfos(vector<ErrorInfo> errors)
{
    m_errors = errors;
    m_currentError = -1;
}

void EditorWindow::goToError(int n)
{
    if (n < 0 || n >= m_errors.size()) return;
    const ErrorInfo& err = m_errors[n];

    openFile(err.m_fileName);
    EditorData& data = getEditor().getData();

    // Make sure we have a valid line
    if ((err.m_line - 1) < data.getNumLines())
    {
        EditorData::Pos pos = data.getPosAtLine(err.m_line - 1);

        // Make sure we have a valid column
        if ((err.m_column - 1) < data.lineLength(err.m_line - 1))
        {
            pos += err.m_column - 1;
        }

        data.moveTo(pos);
    }

    setStatus(err.m_error, Draw::attr(Colour::Black, Colour::Red, true));
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

