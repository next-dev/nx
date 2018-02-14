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

string CommandWindow::extractInput(EditorData& data)
{
    string input;
    int x0 = data.getPosAtLine(data.getCurrentLine());
    SplitView view = data.getText();
    if (view[x0] != ';')
    {
        // Transfer a previous line to the end
        if (view[x0] == '>')
        {
            x0++;
            if (x0 != data.dataLength() && view[x0] == ' ') x0++;
        }

        while (x0 != data.dataLength() && view[x0] != '\n')
        {
            char ch = view[x0++];
            if (ch >= 'a' && ch <= 'z') ch -= 32;
            input.push_back(ch);
        }
    }

    return input;
}

void CommandWindow::registerCommand(string cmd, CommandFunction handler)
{
    m_handlers[cmd] = handler;
}

void CommandWindow::onEnter(Editor& editor)
{
    EditorData& data = editor.getData();
    string input = extractInput(data);

    if (data.getCurrentLine() == editor.getData().getNumLines()-1)
    {
        // Editing on last line!
        data.newline();

        // Extract command
        vector<string> args;
        string arg;
        int i = 0;

        while (input[i] != 0)
        {
            arg.clear();
            while (input[i] != 0 && iswspace(input[i])) ++i;
            while (input[i] != 0 && !iswspace(input[i]))
            {
                arg.push_back(input[i++]);
            }
            if (!arg.empty()) args.push_back(arg);
        }

        // Execute command
        if (!args.empty())
        {
            auto it = m_handlers.find(args[0]);
            if (it != m_handlers.end())
            {
                args.erase(args.begin());
                vector<string> output = it->second(args);
                for (const auto& line : output)
                {
                    data.insert("; ");
                    data.insert(line);
                    data.newline();
                }
            }
        }

        prompt();
    }
    else
    {
        string input = extractInput(data);
        data.moveTo(data.dataLength());
        editor.ensureVisibleCursor();
        data.insert(input);
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
