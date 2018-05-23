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
        "F3|Find Next",
        "Shift-F3|Find Previous",
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
    , m_currentAddress(-1)
{
    m_disassemblyWindow.Select();

    //
    // Help command
    //
    m_commandWindow.registerCommand("?", [this](vector<string> args) -> vector<string> {
        // Known commands
        return {
            "B  <addr>        Toggle breakpoint",
            "DB <addr> <len>  Toggle data breakpoint",
            "LB               List breakpoints",
            "CB               Clear breakpoints",
            "CF               Clear search terms",
            "F  <byte>...     Find byte(s)",
            "FW <word>        Find word",
        };
    });

    //
    // Data breakpoint command
    //

    m_commandWindow.registerCommand("DB", [this](vector<string> args) {
        vector<string> errors = syntaxCheck(args, "w?w", { "DB", "address", "len" });
        if (errors.empty())
        {
            u16 addr = 0;
            u16 len = 1;
            bool parsedAddr = parseWord(args[0], addr);
            bool parsedLen = args.size() == 1 ? true : parseWord(args[1], len);

            if (parsedAddr && parsedLen)
            {
                getSpeccy().toggleDataBreakpoint(addr, len);
                if (len == 1)
                {
                    errors.emplace_back(
                        stringFormat(getSpeccy().hasDataBreakpoint(addr, len)
                            ? "Data breakpoint set at ${0}."
                            : "Data breakpoint reset at ${0}.",
                            hexWord(addr)));
                }
                else
                {
                    errors.emplace_back(
                        stringFormat(getSpeccy().hasDataBreakpoint(addr, len)
                            ? "Data breakpoint set at ${0}-${1}."
                            : "Data breakpoint reset at ${0}-${1}.",
                            hexWord(addr),
                            hexWord(addr + len - 1)));
                }
            }
            else
            {
                if (!parsedAddr) errors.emplace_back(stringFormat("Invalid address: '{0}'.", args[0]));
                if (!parsedLen) errors.emplace_back(stringFormat("Invalid length: '{0}'.", args[1]));
            }
        }

        return errors;
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

        return errors;
    });

    //
    // List breakpoints
    //
    m_commandWindow.registerCommand("LB", [this](vector<string> args) {
        vector<string> errors = syntaxCheck(args, "", { "LB" });
        if (errors.empty())
        {
            errors.emplace_back("Breakpoints:");
            for (const auto& br : getSpeccy().getUserBreakpoints())
            {
                errors.emplace_back(stringFormat("  ${0}", hexWord(br)));
            }
            errors.emplace_back("Data breakpoints:");
            for (const auto& br : getSpeccy().getDataBreakpoints())
            {
                if (br.len == 1)
                {
                    errors.emplace_back(stringFormat("  ${0}", hexWord(br.address)));
                }
                else
                {
                    errors.emplace_back(stringFormat("  ${0}-${1}", hexWord(br.address), hexWord(br.address + br.len - 1)));
                }
            }
        }

        return errors;
    });

    //
    // Clear breakpoints
    //
    m_commandWindow.registerCommand("CB", [this](vector<string> args) {
        vector<string> errors = syntaxCheck(args, "", { "CB" });
        if (errors.empty())
        {
            getSpeccy().clearUserBreakpoints();
            getSpeccy().clearDataBreakpoints();
            errors.emplace_back("Cleared all breakpoints.");
        }

        return errors;
    });

    //
    // Clear command
    //
    m_commandWindow.registerCommand("CF", [this](vector<string> args) {
        vector<string> errors = syntaxCheck(args, "", { "CF" });
        if (errors.empty())
        {
            m_findAddresses.clear();
            m_findWidth = 0;
            m_currentAddress = -1;
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
            if (args.size() == 0)
            {
                m_findAddresses.clear();
                m_currentAddress = -1;
                m_findWidth = 0;
            }
            else
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
                            errors.emplace_back(stringFormat("Argument {0} is the wrong type.", i + 1));
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
                    m_currentAddress = -1;
                    m_findWidth = (int)bytes.size();
                    for (u32 add : m_findAddresses)
                    {
                        errors.push_back(getSpeccy().addressName(add, true));
                    }
                    errors.push_back(stringFormat("{0} address(es) found.", m_findAddresses.size()));
                    errors.push_back("Use F3/Shift-F3 to jump to them in the memory or disassembly view.");
                }
            }
        }

        return errors;
    });
    m_commandWindow.registerCommand("FW", [this](vector<string> args) -> vector<string> {
        vector<string> errors = syntaxCheck(args, "w", { "FW", "word" });
        if (errors.empty())
        {
            u16 w;
            if (args.size() == 0)
            {
                m_findAddresses.clear();
                m_currentAddress = -1;
                m_findWidth = 0;
            }
            else if (parseWord(args[0], w))
            {
                m_findAddresses = getSpeccy().findWord(w);
                m_currentAddress = -1;
                m_findWidth = 2;
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

//
// Syntax check uses a simple VM to parse the arguments:
//
//  Each command is: <modifier>?<type>
//
//  Where modifier is:
//
//      '?': type is optional
//      '*': can have 0 or more types
//      '+': can have 1 or more types
//
//  Where type is:
//
//      'w': 16-bit word
//      'b': 8-bit byte
//      's': 8-bit bytes or strings
//

vector<string> Debugger::syntaxCheck(const vector<string>& args, const char* format, vector<string> desc)
{
    vector<string> errors;

    int fi = 0;
    int ai = 0;

    enum class SyntaxState
    {
        Normal,
        Optional,
        Star,
        Plus,
    } state = SyntaxState::Normal;

    for (const string& arg : args)
    {
        ++ai;

        //
        // Check for mods first
        //
        if (format[fi] == '?' ||
            format[fi] == '*' ||
            format[fi] == '+')
        {
            switch (format[fi++])
            {
            case '?': state = SyntaxState::Optional;    break;
            case '*': state = SyntaxState::Star;        break;
            case '+': state = SyntaxState::Plus;        break;

            case '\0':
                // End of format - too soon!
                errors.emplace_back("Too few arguments!");
                break;
            }

            if (errors.empty()) continue;
        }

        enum class ArgType
        {
            Error,
            Integer,
            String,
        };

        //
        // Figure out the type
        //
        ArgType type = ArgType::Error;
        int t = 0;

        if (arg[0] == '$' || (arg[0] >= '0' && arg[0] <= '9'))
        {
            // Value is hexadecimal
            if (!parseNumber(arg, t))
            {
                errors.emplace_back(stringFormat("Argument {0} is invalid: '{1}'.", ai, arg));
                break;
            }
            type = ArgType::Integer;
        }
        else if (arg[0] == '"')
        {
            type = ArgType::String;
        }
        else
        {
            errors.emplace_back(stringFormat("Argument {0} is invalid: '{1}'.", ai, arg));
            break;
        }

        bool validArg = true;
        switch (format[fi])
        {
        case 'w':
        case 'b':
            {
                if (type == ArgType::Integer)
                {
                    int maxValue = format[fi] == 'w' ? 65535 : 255;
                    if (t < 0 || t > maxValue)
                    {
                        //errors.emplace_back(stringFormat("Argument {0} is too large.", ai));
                        validArg = false;
                        break;
                    }
                }
                else
                {
                    validArg = false;
                }
            }
            break;

        case 's':
            {
                if (type == ArgType::Integer)
                {
                    if (t > 255)
                    {
                        validArg = false;
                        break;
                    }
                }
                else if (type != ArgType::String)
                {
                    validArg = false;
                }
            }
            break;

        default:
            NX_ASSERT(0);
        }

        //
        // Check if we can be invalid or not
        //
        if (validArg)
        {
            switch (state)
            {
            case SyntaxState::Normal:   ++fi;                               break;
            case SyntaxState::Optional: state = SyntaxState::Normal; ++fi;  break;
            case SyntaxState::Plus:     state = SyntaxState::Star;          break;
            case SyntaxState::Star:                                         break;
            }
        }
        else
        {
            switch(state)
            {
            case SyntaxState::Normal:
                errors.emplace_back(stringFormat("Argument {0} is invalid: '{1}'.", ai, arg));
                ++fi;
                break;

            case SyntaxState::Optional:
            case SyntaxState::Star:
                state = SyntaxState::Normal;
                ++fi;
                break;

            case SyntaxState::Plus:
                errors.emplace_back(stringFormat("Argument {0} is invalid: '{1}'.", ai, arg));
                state = SyntaxState::Normal;
                ++fi;
                break;
            }
        }

        //
        // If we have errors, describe the command
        //
        if (!errors.empty())
        {
            // An error occurred
            vector<string> lines = describeCommand(format, desc);
            errors.insert(errors.end(), lines.begin(), lines.end());
            break;
        }
    }

    // #todo: Check to see if the format is complete
    if (errors.empty())
    {

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
            if (m_disassemblyWindow.allowExit() && m_memoryDumpWindow.allowExit())
            {
                getEmulator().toggleDebugger();
            }
            else
            {
                SelectableWindow::getSelected().keyPress(key, down, shift, ctrl, alt);
            }
            break;

        case K::F1:
            getSpeccy().renderVideo();
            break;

        case K::F3:
            if (!m_findAddresses.empty())
            {
                if (++m_currentAddress >= m_findAddresses.size()) m_currentAddress = 0;
                m_memoryDumpWindow.gotoAddress(m_findAddresses[m_currentAddress]);
            }
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
    else if (shift && !ctrl && !alt)
    {
        switch (key)
        {
        case K::F3:
            if (!m_findAddresses.empty())
            {
                if (--m_currentAddress < 0) m_currentAddress = max(0, (int)m_findAddresses.size() - 1);
                m_memoryDumpWindow.gotoAddress(m_findAddresses[m_currentAddress]);
            }
            break;

        default:
            SelectableWindow::getSelected().keyPress(key, down, shift, ctrl, alt);
        }
    }
    else if (!shift && !ctrl && alt)
    {
        switch (key)
        {
        case K::Num1:
            getDisassemblyWindow().Select();
            break;

        case K::Num2:
            getMemoryDumpWindow().Select();
            break;

        case K::Num3:
            getCommandWindow().Select();
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
