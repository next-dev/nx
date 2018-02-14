//----------------------------------------------------------------------------------------------------------------------
// Command window
//----------------------------------------------------------------------------------------------------------------------

#include <debugger/overlay_debugger.h>
#include <emulator/nx.h>

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

CommandWindow::CommandWindow(Nx& nx)
    : SelectableWindow(nx, 45, 22, 34, 30, "Command Window", Colour::Blue, Colour::Black)
    , m_commandEditor(46, 23, 32, 28, Draw::attr(Colour::White, Colour::Black, false), false, 1024, 1024,
        bind(&CommandWindow::onEnter, this, placeholders::_1))
{
    prompt();
    m_commandEditor.setCommentColour(Draw::attr(Colour::Green, Colour::Black, false));
}

void CommandWindow::prompt()
{
    m_commandEditor.getData().insert("> ");
}

//----------------------------------------------------------------------------------------------------------------------
// Window overrides
//----------------------------------------------------------------------------------------------------------------------

void CommandWindow::onDraw(Draw& draw)
{
    m_commandEditor.renderAll(draw);
}

void CommandWindow::onKey(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    if (down && !shift && ctrl && !alt && key == sf::Keyboard::L)
    {
        m_commandEditor.clear();
        prompt();
    }
    m_commandEditor.key(key, down, shift, ctrl, alt);
}

void CommandWindow::onText(char ch)
{
    m_commandEditor.text(ch);
}

//----------------------------------------------------------------------------------------------------------------------
// Editor reaction
//----------------------------------------------------------------------------------------------------------------------

void CommandWindow::onEnter(Editor& editor)
{
    EditorData& data = editor.getData();
    if (data.getCurrentLine() == editor.getData().getNumLines()-1)
    {
        // Editing on last line!
        data.newline();
        data.insert("; Command accepted");
        data.newline();
        prompt();
    }
    else
    {
        int x0 = data.getPosAtLine(data.getCurrentLine());
        SplitView view1 = data.getText();
        if (view1[x0] != ';')
        {
            // Transfer a previous line to the end
            if (view1[x0] == '>')
            {
                x0++;
                if (view1[x0] == ' ') x0++;
            }

            data.moveTo(data.dataLength());
            editor.ensureVisibleCursor();
            SplitView view2 = data.getText();
            while (view2[x0] != '\n')
            {
                data.insert(view2[x0++]);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
