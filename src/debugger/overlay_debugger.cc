//----------------------------------------------------------------------------------------------------------------------
// Debugger
//----------------------------------------------------------------------------------------------------------------------

#include <debugger/overlay_debugger.h>
#include <emulator/nx.h>
#include <utils/format.h>

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Debugger::Debugger(Nx& nx)
    : Overlay(nx)
    , m_memoryDumpWindow(nx)
    , m_disassemblyWindow(nx)
    , m_cpuStatusWindow(nx)
    , m_commandWindow(nx)
    , m_memoryDumpCommands({
        "G|oto",
        "C|hecksums",
        "E|dit",
        "Up|Scroll up",
        "Down|Scroll down",
        "PgUp|Page up",
        "PgDn|Page down",
        "~|Exit",
        "Tab|Switch window",
        "Ctrl-Z|Toggle Zoom",
        })
    , m_disassemblyCommands({
        "G|oto",
        "F1|Render video",
        "F5|Pause/Run",
        "Ctrl-F5|Run to",
        "F6|Step Over",
        "F7|Step In",
        "F8|Step Out",
        "F9|Breakpoint",
        "Up|Scroll up",
        "Down|Scroll down",
        "PgUp|Page up",
        "PgDn|Page down",
        "~|Exit",
        "Tab|Switch window",
        "Ctrl-Z|Toggle Zoom",
        })
    , m_cliCommands({
        "~|Exit",
        "Tab|Switch window",
        "Ctrl-Z|Toggle Zoom"
        })
    , m_zoomMode(false)
{
    m_disassemblyWindow.Select();

    //
    // Help command
    //
    m_commandWindow.registerCommand("?", [this](vector<string> args) -> vector<string> {
        // Known commands
        return {
            "B   Toggle breakpoint",
            "F   Find byte(s)",
            "FW  Find word",
        };
    });

    //
    // Breakpoint command
    //
    m_commandWindow.registerCommand("B", [this](vector<string> args) {
        vector<string> errors = syntaxCheck(args, "w", { "B", "address" });
        if (errors.empty())
        {
            u16 addr = 0;
            if (parseWord(args[0], addr))
            {
                getSpeccy().toggleBreakpoint(addr);
                errors.emplace_back(
                    stringFormat(getSpeccy().hasUserBreakpointAt(addr)
                        ? "Breakpoint set at ${0}."
                        : "Breakpoint reset at ${0}.", hexWord(addr)));
            }
            else
            {
                errors.emplace_back(stringFormat("Invalid address: '{0}'.", args[0]));
            }
        }
        else
        {
            return errors;
        }

        return errors;
    });

    //
    // Find commands
    //
    m_commandWindow.registerCommand("F", [this](vector<string> args) -> vector<string> {
        vector<string> desc = { "F", "byte/string" };
        vector<string> errors = syntaxCheck(args, "+s", desc);
        if (errors.empty())
        {
            vector<u8> bytes;
            u8 b;
            for (int i = 0; i < args.size(); ++i)
            {
                if (parseByte(args[i], b))
                {
                    bytes.push_back(b);
                }
                else
                {
                    u16 w;
                    if (parseWord(args[i], w))
                    {
                        // Catch any value that is not a byte and isn't a string
                        errors.emplace_back(stringFormat("Argument {0} is the wrong type.", i+1));
                        vector<string> description = describeCommand("+s", desc);
                        errors.insert(errors.end(), description.begin(), description.end());
                        break;
                    }
                    for (auto c : args[i]) bytes.push_back(c);
                }
            }

            if (bytes.size() > 0)
            {
                m_findAddresses = getSpeccy().findSequence(bytes);
                for (u32 add : m_findAddresses)
                {
                    errors.push_back(getSpeccy().addressName(add, true));
                }
                errors.push_back(stringFormat("{0} address(es) found.", m_findAddresses.size()));
                errors.push_back("Use F3/Shift-F3 to jump to them in the memory or disassembly view.");
            }
        }

        return errors;
    });
    m_commandWindow.registerCommand("FW", [this](vector<string> args) -> vector<string> {
        vector<string> errors = syntaxCheck(args, "w", { "FW", "word" });
        if (errors.empty())
        {
            u16 w;
            if (parseWord(args[0], w))
            {
                m_findAddresses = getSpeccy().findWord(w);
                for (u32 add : m_findAddresses)
                {
                    errors.push_back(getSpeccy().addressName(add, true));
                }
                errors.push_back(stringFormat("{0} address(es) found.", m_findAddresses.size()));
                errors.push_back("Use F3/Shift-F3 to jump to them in the memory or disassembly view.");
            }
            else
            {
                errors.push_back("Invalid parameter.");
            }
        }

        return errors;
    });
}

//----------------------------------------------------------------------------------------------------------------------
// Command handling
//----------------------------------------------------------------------------------------------------------------------

vector<string> Debugger::syntaxCheck(const vector<string>& args, const char* format, vector<string> desc)
{
    vector<string> errors;
    char plus = 0;
    char star = 0;

    // Check number of arguments
    for (int i = 0, count = 0; format[i] != 0;)
    {
        // Check overflow of arguments
        if (count >= args.size())
        {
            if (!plus && !star) errors.emplace_back(stringFormat("Bad number of args."));
            break;
        }

        const string& arg = args[count];
        switch (format[i])
        {
        case 'w':   // Address expected
        case 'b':   // Byte expected
            {
                if (arg[0] == '$')
                {
                    // Value is hexadecimal
                    int h = 1;
                    for (; arg[h] != 0; ++h)
                    {
                        if (!isxdigit(arg[h]))
                        {
                            if (!star) errors.emplace_back(stringFormat("Invalid hex value for argument {0}.", i+1));
                            star = 0;
                            break;
                        }
                    }
                    if (h > (format[i] == 'w' ? 5 : 3))
                    {
                        if (!star) errors.emplace_back(stringFormat("Argument {0} too large.", i+1));
                        star = 0;
                    }
                }
                else
                {
                    // Value is decimal
                    int t = 0;
                    for (int d = 0; arg[d] != 0; ++d)
                    {
                        if (!isdigit(arg[d]))
                        {
                            if (!star) errors.emplace_back(stringFormat("Invalid decimal value for argument {0}.", i+1));
                            star = 0;
                            break;
                        }
                        t *= 10;
                        t += arg[d] - '0';
                    }
                    if (t > 65535)
                    {
                        if (!star) errors.emplace_back(stringFormat("Argument {0} too large.", i+1));
                        star = 0;
                    }
                }
            }
            ++count;
            break;

        case 's':
            ++count;
            break;

        case '+':
            assert(plus == 0);
            assert(star == 0);
            plus = format[i+1];
            break;

        case '*':
            assert(plus == 0);
            assert(star == 0);
            star = format[++i];
            break;

        default:
            assert(0);
            break;
        }

        if (!errors.empty()) break;

        if (plus)
        {
            if (format[i] == '+')
            {
                ++i;
            }
            else
            {
                star = plus;
                plus = 0;
            }
        }
        if (!plus && !star)
        {
            ++i;
        }
    }

    if (!errors.empty())
    {
        // An error occurred
        vector<string> lines = describeCommand(format, desc);
        errors.insert(errors.end(), lines.begin(), lines.end());
    }

    return errors;
}

vector<string> Debugger::describeCommand(const char* format, vector<string> desc)
{
    vector<string> lines;
    lines.emplace_back("Syntax:");
    string syntax = " " + desc[0];
    for (int i = 1; i < desc.size(); ++i)
    {
        syntax += string(" <") + desc[i] + ">";
    }
    lines.emplace_back(syntax);
    return lines;
}

//----------------------------------------------------------------------------------------------------------------------
// Keyboard handling
//----------------------------------------------------------------------------------------------------------------------

void Debugger::key(sf::Keyboard::Key key, bool down, bool shift, bool ctrl, bool alt)
{
    using K = sf::Keyboard::Key;
    if (!down) return;

    if (!shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::Tilde:
        case K::Escape:
            getEmulator().toggleDebugger();
            break;

        case K::F1:
            getSpeccy().renderVideo();
            break;

        case K::F5:
            getEmulator().togglePause(false);
            break;

        case K::F6:
            getEmulator().stepOver();
            break;

        case K::F7:
            getEmulator().stepIn();
            break;

        case K::F8:
            getEmulator().stepOut();
            break;

        case K::Tab:
            // #todo: put cycling logic into SelectableWindow
            if (getDisassemblyWindow().isSelected())
            {
                getMemoryDumpWindow().Select();
            }
            else if (getMemoryDumpWindow().isSelected())
            {
                getCommandWindow().Select();
            }
            else
            {
                getDisassemblyWindow().Select();
            }
            break;

        default:
            SelectableWindow::getSelected().keyPress(key, down, shift, ctrl, alt);
        }
    }
    else if (!shift && ctrl && !alt)
    {
        switch (key)
        {
        case K::Z:
            m_zoomMode = !m_zoomMode;
            break;

        default:
            SelectableWindow::getSelected().keyPress(key, down, shift, ctrl, alt);
        }
    }
    else
    {
        SelectableWindow::getSelected().keyPress(key, down, shift, ctrl, alt);
    }
}

void Debugger::text(char ch)
{
    if (ch != '`')
    {
        SelectableWindow::getSelected().text(ch);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------------------------------------------------

void Debugger::render(Draw& draw)
{
    m_memoryDumpWindow.draw(draw);
    m_cpuStatusWindow.draw(draw);

    if (m_zoomMode)
    {
        m_memoryDumpWindow.zoomMode(true);
        m_disassemblyWindow.zoomMode(true);
        m_commandWindow.zoomMode(true);

        if (m_memoryDumpWindow.isSelected())
        {
            m_commandWindow.zoomMode(false);
            m_commandWindow.draw(draw);
        }
        else if (m_disassemblyWindow.isSelected())
        {
            m_memoryDumpWindow.zoomMode(false);
            m_disassemblyWindow.draw(draw);
        }
        else if (m_commandWindow.isSelected())
        {
            m_memoryDumpWindow.zoomMode(false);
            m_commandWindow.draw(draw);
        }
    }
    else
    {
        m_memoryDumpWindow.zoomMode(false);
        m_disassemblyWindow.zoomMode(false);
        m_commandWindow.zoomMode(false);
        m_disassemblyWindow.draw(draw);
        m_commandWindow.draw(draw);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Commands
//----------------------------------------------------------------------------------------------------------------------

const vector<string>& Debugger::commands() const
{
    if (m_memoryDumpWindow.isSelected())
    {
        return m_memoryDumpCommands;
    }
    else if (m_disassemblyWindow.isSelected())
    {
        return m_disassemblyCommands;
    }
    else
    {
        return m_cliCommands;
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
