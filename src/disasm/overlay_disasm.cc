//----------------------------------------------------------------------------------------------------------------------
// Disassembler
//----------------------------------------------------------------------------------------------------------------------

#include <disasm/overlay_disasm.h>
#include <emulator/nx.h>
#include <utils/tinyfiledialogs.h>
#include <utils/format.h>

#define K_LINE_SKIP 20

//----------------------------------------------------------------------------------------------------------------------
// DisassemblerEditor
//----------------------------------------------------------------------------------------------------------------------

DisassemblerEditor::DisassemblerEditor(Spectrum& speccy, int xCell, int yCell, int width, int height)
    : m_speccy(&speccy)
    , m_data(speccy)
    , m_x(xCell)
    , m_y(yCell)
    , m_width(width)
    , m_height(height)
    , m_topLine(0)
    , m_lineOffset(0)
    , m_editor(nullptr)
    , m_blockFirstChar(false)
    , m_currentLine(0)
{

}

void DisassemblerEditor::ensureVisibleCursor()
{
    // Check for required up scroll
    if (m_currentLine < m_topLine)
    {
        m_topLine = max(0, m_topLine - K_LINE_SKIP);
    }

    // Check for required down scroll
    else if (m_currentLine >= (m_topLine + m_height))
    {
        m_topLine = min(getData().getNumLines() - 1, m_topLine + K_LINE_SKIP);

    }

    // if still not visible, then snap
    if ((m_currentLine < m_topLine) || (m_currentLine >= (m_topLine + m_height)))
    {
        m_topLine = max(0, m_currentLine - (m_height / 2));
    }
}

void DisassemblerEditor::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    if (!down) return;

    if (!m_editor)
    {

        //------------------------------------------------------------------------------------------------------------------
        // No shift keys
        //------------------------------------------------------------------------------------------------------------------

        if (!shift && !ctrl && !alt)
        {
            switch (key)
            {
            case K::Up:
                if (m_currentLine > 0) --m_currentLine;
                ensureVisibleCursor();
                break;

            case K::Down:
                if (m_currentLine < getData().getNumLines() - 1) ++m_currentLine;
                break;

            case K::Left:
                m_lineOffset = max(0, m_lineOffset - 1);
                break;

            case K::Right:
                m_lineOffset = min(max(0, int(m_longestLine - 2)), m_lineOffset + 1);
                break;

            case K::PageUp:
                m_currentLine = min(m_currentLine + m_height, getData().getNumLines() - 1);
                ensureVisibleCursor();
                break;

            case K::PageDown:
                m_currentLine = max(0, m_currentLine - m_height);
                ensureVisibleCursor();
                break;

            case K::Home:
                if (m_lineOffset != 0)
                {
                    m_lineOffset = 0;
                }
                else
                {
                    m_topLine = 0;
                }
                ensureVisibleCursor();
                break;

            case K::End:
                m_currentLine = getData().getNumLines() - 1;
                ensureVisibleCursor();
                break;

            case K::Delete:
                m_currentLine = getData().deleteLine(m_currentLine);
                m_currentLine = max(0, min(m_currentLine, getData().getNumLines() - 1));
                ensureVisibleCursor();
                break;

            case K::SemiColon:
                if ((getData().getNumLines() == 0) ||
                    (getData().getLine(m_currentLine).type != DisassemblerDoc::LineType::Instruction))
                {
                    if ((m_currentLine < getData().getNumLines()) &&
                        (getData().getLine(m_currentLine).type == DisassemblerDoc::LineType::FullComment))
                    {
                        // Edit full line comment
                        m_blockFirstChar = true;
                        m_editorPrefix.clear();
                        m_editor = new Editor(m_x + 3, m_y + (m_currentLine - m_topLine), m_width - 4, 1,
                            Draw::attr(Colour::Green, Colour::Black, true), false, m_width - 5, 0,
                            [this](Editor& ed)
                            {
                                // Reset the command text
                                getData().setComment(m_currentLine, ed.getData().getString());
                            });
                    }
                    else
                    {
                        getData().insertComment(m_currentLine, getData().getNextTag(), "");
                        m_blockFirstChar = true;
                        m_editorPrefix.clear();
                        m_editor = new Editor(m_x + 3, m_y + (m_currentLine - m_topLine), m_width - 4, 1,
                            Draw::attr(Colour::Green, Colour::Black, true), false, m_width - 5, 0,
                            [this](Editor& ed)
                            {
                                getData().setComment(m_currentLine, ed.getData().getString());
                                ++m_currentLine;
                            });
                        }
                }
                break;

            default:
                break;
            }
        }

        //------------------------------------------------------------------------------------------------------------------
        // Ctrl-keys
        //------------------------------------------------------------------------------------------------------------------

        else if (!shift && ctrl && !alt)
        {
            switch (key)
            {
            case K::Home:
                m_topLine = 0;
                break;

            case K::End:
                m_topLine = max(0, getData().getNumLines() - (m_height / 2));
                break;

            case K::S:  // Save
                saveFile();
                break;

            default:
                break;
            }
        }

        //--------------------------------------------------------------------------------------------------------------
        // Shift-keys
        //--------------------------------------------------------------------------------------------------------------

        else if (shift && !ctrl && !alt)
        {
            switch (key)
            {
            case K::SemiColon:
                m_blockFirstChar = true;
                m_editorPrefix.clear();
                m_editor = new Editor(m_x + 3, m_y + (m_currentLine - m_topLine), m_width - 4, 1,
                    Draw::attr(Colour::Green, Colour::Black, true), false, m_width - 5, 0,
                    [this](Editor& ed)
                {
                    getData().insertComment(m_currentLine, getData().getNextTag(), ed.getData().getString());
                    ++m_currentLine;
                });
                break;

            default:
                break;
            }
        }
    }
    else
    {
        // Editing mode
        m_editor->key(key, down, shift, ctrl, alt);
    }
}

void DisassemblerEditor::onText(char ch)
{
    if (!m_blockFirstChar && m_editor)
    {
        m_editor->text(ch);
    }
    m_blockFirstChar = false;

    if (m_editor && ch == 13)
    {
        delete m_editor;
        m_editor = nullptr;
    }
}

string DisassemblerEditor::getTitle() const
{
    string title = getFileName();
    if (title.empty()) title = "[new file]";
    if (m_data.hasChanged()) title += "*";
    return title;
}

void DisassemblerEditor::render(Draw& draw)
{
    m_longestLine = 0;

    // Colours
    u8 bkgColour = Draw::attr(Colour::White, Colour::Black, false);
    u8 commentColour = Draw::attr(Colour::Green, Colour::Black, true);
    u8 labelColour = Draw::attr(Colour::Cyan, Colour::Black, true);
    u8 rangeColour = Draw::attr(Colour::Black, Colour::Green, false);
    u8 cursorColour = Draw::attr(Colour::White, Colour::Blue, true) | 0x80;
    u8 cursor2Colour = Draw::attr(Colour::White, Colour::Black, false);

    int y = m_y;
    if (getData().getNumLines() > 0)
    {
        int tag = m_currentLine < getData().getNumLines() ? getData().getLine(m_currentLine).tag : -1;
        for (int i = m_topLine; i < getData().getNumLines(); ++i, ++y)
        {
            using T = DisassemblerDoc::LineType;
            const DisassemblerDoc::Line& line = getData().getLine(i);

            switch (line.type)
            {
            case T::Blank:
                break;

            case T::FullComment:
                m_longestLine = max(m_longestLine, (int)line.text.size());
                draw.printChar(m_x + 1, y, ';', commentColour);
                draw.printString(m_x + 3, y, line.text, false, commentColour);
                break;

            case T::Label:
                m_longestLine = max(m_longestLine, (int)line.text.size() + 1);
                draw.printString(m_x + 1, y, line.text + ":", false, labelColour);
                break;

            case T::Instruction:
                m_longestLine = max(m_longestLine, 32 + (int)line.operand.size());
                draw.printString(m_x + 9, y, line.opCode, false, bkgColour);
                draw.printString(m_x + 15, y, line.operand, false, bkgColour);
                if (!line.text.empty()) draw.printString(m_x + 33, y, string("; ") + line.text, false, commentColour);
                break;
            }

            if (i == m_currentLine)
            {
                draw.printChar(m_x, y, '*', cursorColour, gGfxFont);
            }
            else if (tag == line.tag)
            {
                draw.printChar(m_x, y, '*', cursor2Colour, gGfxFont);
            }
        }
    }

    if (m_currentLine == getData().getNumLines())
    {
        draw.printChar(m_x, y, '*', cursorColour, gGfxFont);
    }

    if (m_editor)
    {
        if (!m_editorPrefix.empty())
        {
            draw.printString(m_editor->getX() - (int)m_editorPrefix.size(), m_editor->getY(), m_editorPrefix, false,
                m_editor->getBkgColour());
        }
        m_editor->renderAll(draw);
    }
}

void DisassemblerEditor::saveFile()
{
    string fileName = getFileName();

    if (fileName.empty())
    {
        const char* filters[] = { "*.dis" };
        const char* fn = tinyfd_saveFileDialog("Save source code", 0, sizeof(filters) / sizeof(filters[0]),
            filters, "Disassembly");
        fileName = fn ? fn : "";
    }
    if (!fileName.empty())
    {
        // If there is no '.' after the last '\', if there is no '.' and no '\\', we have no extension.
        // Add .asm in this case.
        std::string finalName = fileName;
        char* slashPos = strrchr((char *)fileName.c_str(), '\\');
        char* dotPos = strrchr((char *)fileName.c_str(), '.');
        if ((slashPos && !dotPos) ||
            (slashPos && (dotPos < slashPos)) ||
            (!slashPos && !dotPos))
        {
            finalName += ".dis";
        }

        if (!getData().save(finalName.c_str()))
        {
            tinyfd_messageBox("ERROR", "Unable to open file!", "ok", "warning", 0);
        }
        else
        {
            setFileName(finalName);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// DisassemblerWindow
//----------------------------------------------------------------------------------------------------------------------

DisassemblerWindow::DisassemblerWindow(Nx& nx)
    : Window(nx, 1, 1, 78, 59, "Disassembler", Colour::Blue, Colour::Black, false)
    ,  m_selectedTab(-1)
{

}

void DisassemblerWindow::onDraw(Draw& draw)
{
    if (m_editors.empty())
    {
        string line1 = "Press {Ctrl-O} to open a disassembly session for editing";
        string line2 = "Press {Ctrl-N} to create a new disassembly session";
        u8 colour = draw.attr(Colour::White, Colour::Black, false);

        int y = m_y + (m_height / 2);

        draw.printString(m_x + (m_width - int(line1.size())) / 2, y - 1, line1, true, colour);
        draw.printString(m_x + (m_width - int(line2.size())) / 2, y + 1, line2, true, colour);
    }
    else
    {
        //
        // Draw the current editor
        //
        getEditor().render(draw);

        //
        // If Ctrl+Tab is pressed draw the menu
        //
        if (m_selectedTab >= 0)
        {
            int maxWidth = 0;
            for (size_t i = 0; i < m_editorOrder.size(); ++i)
            {
                DisassemblerEditor& editor = m_editors[m_editorOrder[i]];
                int width = draw.squashedStringWidth(editor.getTitle());
                maxWidth = std::max(width + 2, maxWidth);
            }

            maxWidth = std::max(20, maxWidth);
            draw.window(m_x + 1, m_y + 1, maxWidth + 2, int(m_editorOrder.size() + 2), "Buffers", true);

            for (size_t i = 0; i < m_editorOrder.size(); ++i)
            {
                u8 colour = i == m_selectedTab ? Draw::attr(Colour::White, Colour::Red, true) : Draw::attr(Colour::Black, Colour::White, true);
                DisassemblerEditor& editor = m_editors[m_editorOrder[i]];
                for (int x = 0; x < maxWidth; ++x)
                {
                    draw.printChar(m_x + 2 + x, m_y + 2 + int(i), ' ', colour);
                }
                draw.printSquashedString(m_x + 2, m_y + 2 + int(i), editor.getTitle(), colour);
            }
        }
    }
}

void DisassemblerWindow::newFile()
{
    int index = int(m_editors.size());
    m_editors.emplace_back(m_nx.getSpeccy(), 2, 2, 76, 57);
    m_editorOrder.insert(m_editorOrder.begin(), index);
}

void DisassemblerWindow::closeFile()
{
    if (m_editors.empty()) return;

    if (getEditor().getData().hasChanged())
    {
        // Check to see if the user really wants to overwrite their changes
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
}

void DisassemblerWindow::openFile(const string& fileName /* = string() */)
{
    const char* fn = 0;
    if (fileName.empty())
    {
        // We need to ask for a filename
        const char* filters[] = { "*.dis" };
        fn = tinyfd_openFileDialog("Load disassembly", 0, sizeof(filters) / sizeof(filters[0]), filters, "Disassembly", 0);
    }
    else
    {
        fn = fileName.c_str();
    }

    if (fn)
    {
        // Make sure we don't already have it open
        for (int i = 0; i < getNumEditors(); ++i)
        {
            if (getEditor(i).getFileName() == fn)
            {
                switchTo(getEditor(i));
                return;
            }
        }

        newFile();

        DisassemblerEditor& thisEditor = getEditor();
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
}

void DisassemblerWindow::switchTo(const DisassemblerEditor& editor)
{
    int editorIndex = int(&editor - m_editors.data());
    int tabIndex = int(find(m_editorOrder.begin(), m_editorOrder.end(), editorIndex) - m_editorOrder.begin());
    m_editorOrder.erase(m_editorOrder.begin() + tabIndex);
    m_editorOrder.insert(m_editorOrder.begin(), editorIndex);
}

void DisassemblerWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    if (getNumEditors() != 0 && getEditor().isEditing()) return;

    //
    // Normal
    //
    if (down && !shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::C:      // Code entry point
            prompt("Code entry", [this](string text) {
                if (optional<MemAddr> addr = m_nx.textToAddress(text); addr)
                {
                    MemAddr a = *addr;
                    if (m_nx.getSpeccy().isZ80Address(*addr))
                    {
                        prompt("Label", [this, a](string label) {
                            getEditor().getData().generateCode(a, getEditor().getData().getNextTag(), label);
                        }, false);
                    }
                }
            }, true);
            break;

        default:
            break;
        }
    }

    //
    // Ctrl
    //

    else if (down && ctrl && !shift && !alt)
    {
        switch (key)
        {
        case K::N:      // New file
            newFile();
            break;

        case K::W:      // Close file
            closeFile();
            break;

        case K::O:      // Open file
            openFile();
            break;
                
        default: break;
        }
    }

    //
    // Ctrl-TAB
    //
    if (!m_editors.empty())
    {
        if (m_selectedTab == -1) getEditor().onKey(key, down, shift, ctrl, alt);
        if (down && ctrl && !shift && !alt)
        {
            if (key == K::Tab)
            {
                if (m_selectedTab == -1)
                {
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
            switchTo(getEditor(index));
            m_selectedTab = -1;
        }

        // Set the title
        setTitle(string("Disassembler - ") + getEditor().getTitle());
    }
    else
    {
        setTitle("Disassembler");
    }
}

void DisassemblerWindow::onText(char ch)
{
    if (!m_editors.empty())
    {
        getEditor().onText(ch);
    }
}

bool DisassemblerWindow::saveAll()
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
            editor.getData().save(editor.getFileName());
        }
    }

    return true;
}

bool DisassemblerWindow::needToSave() const
{
    for (auto& editor : m_editors)
    {
        if (editor.getData().hasChanged()) return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
// DisassemblerOverlay
//----------------------------------------------------------------------------------------------------------------------

DisassemblerOverlay::DisassemblerOverlay(Nx& nx)
    : Overlay(nx)
    , m_window(nx)
    , m_commands({
        "ESC|Exit",
        "Ctrl-S|Save",
        "Ctrl-O|Open",
        "Shift-Ctrl-S|Save as",
        "Ctrl-Tab|Switch buffers",
        "Ctrl-B|Build",
        ";|Add/Edit comment",
        "Ctrl-;|Add/Edit line comment",
        "Shift-;|Insert comment",
        "C|Add code entry point",
        })
{
}

void DisassemblerOverlay::render(Draw& draw)
{
    m_window.draw(draw);
}

void DisassemblerOverlay::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;

    m_window.keyPress(key, down, shift, ctrl, alt);

    if (down && !shift && !ctrl && !alt)
    {
        if (key == K::Escape)
        {
            getEmulator().hideAll();
        }
    }
    else if (down && !shift && ctrl && !alt)
    {
        //bool buildSuccess = false;

        if (key == K::B)
        {
            // Generate the files.
        }
    }
}

void DisassemblerOverlay::text(char ch)
{
    m_window.text(ch);
}

const vector<string>& DisassemblerOverlay::commands() const
{
    return m_commands;
}
