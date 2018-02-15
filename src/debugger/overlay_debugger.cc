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
            "FB  Find byte",
            "FW  Find word",
            "FS  Find string",
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
    m_commandWindow.registerCommand("FB", [this](vector<string> args) -> vector<string> {
        vector<string> errors = syntaxCheck(args, "b", { "FB", "byte" });
        if (errors.empty())
        {
            u8 b;
            if (parseByte(args[0], b))
            {
                m_findAddresses = getSpeccy().findByte(b);
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
    m_commandWindow.registerCommand("FS", [this](vector<string> args) -> vector<string> {
        vector<string> errors = syntaxCheck(args, "s", { "FS", "string" });
        if (errors.empty())
        {
            m_findAddresses = getSpeccy().findString(args[0]);
            for (u32 add : m_findAddresses)
            {
                errors.push_back(getSpeccy().addressName(add, true));
            }
            errors.push_back(stringFormat("{0} address(es) found.", m_findAddresses.size()));
            errors.push_back("Use F3/Shift-F3 to jump to them in the memory or disassembly view.");
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

    // Check number of arguments
    if (args.size() == strlen(format))
    {
        for (int i = 0; format[i] != 0; ++i)
        {
            const string& arg = args[i];
            switch (format[i])
            {
            case 'w':   // Address expected
            case 'b':   // Byte expected
                {
                    if (arg[0] == '$')
                    {
                        // Value is hexadecimal
                        int h = 0;
                        for (; arg[h + 1] != 0; ++h)
                        {
                            if (!isxdigit(arg[h + 1]))
                            {
                                errors.emplace_back(stringFormat("Invalid hex value for argument {0}.", i+1));
                                break;
                            }
                        }
                        if (h > (format[i] == 'w' ? 4 : 2))
                        {
                            errors.emplace_back(stringFormat("Argument {0} too large.", i+1));
                        }
                    }
                    else
                    {
                        // Value is decimal
                        int t = 0;
                        for (int d = 0; arg[d + 1] != 0; ++d)
                        {
                            if (!isdigit(arg[d + 1]))
                            {
                                errors.emplace_back(stringFormat("Invalid decimal value for argument {0}.", i+1));
                                break;
                            }
                            t *= 10;
                            t += arg[d+1] - '0';
                        }
                        if (t > 65535)
                        {
                            errors.emplace_back(stringFormat("Argument {0} too large.", i+1));
                        }
                    }
                }
                break;

            case 's':
                break;

            default:
                assert(0);
                break;
            }
        }
    }
    else
    {
        errors.emplace_back(stringFormat("Bad number of args."));
        errors.emplace_back(stringFormat("Expected {0}, got {1}.", strlen(format), args.size()));
    }

    if (!errors.empty())
    {
        // An error occurred
        errors.emplace_back("Syntax:");
        string syntax = " " + desc[0];
        for (int i = 1; i < desc.size(); ++i)
        {
            syntax += string(" <") + desc[i] + ">";
        }
        errors.emplace_back(syntax);
    }

    return errors;
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
