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
        "Tab|Switch window"})
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
        "Tab|Switch window"})
{
    m_disassemblyWindow.Select();

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
                        ? "Breakpoint set at {0}."
                        : "Breakpoint reset at {0}.", hexWord(addr)));
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

string Debugger::decimalWord(u16 x)
{
    string s;
    int div = 10000;
    while (div > 1 && (x / div == 0)) div /= 10;

    while (div != 0)
    {
        int d = x / div;
        x %= div;
        div /= 10;
        s.push_back('0' + d);
    }

    return s;
}

string Debugger::decimalByte(u8 x)
{
    return decimalWord(u16(x));
}

string Debugger::hexWord(u16 x)
{
    string s;
    int div = 4096;

    while (div != 0)
    {
        int d = x / div;
        x %= div;
        div /= 16;
        s.push_back(d < 10 ? '0' + d : 'A' + d - 10);
    }

    return s;
}

string Debugger::hexByte(u8 x)
{
    return hexWord(u16(x));
}

bool Debugger::parseNumber(const string& str, int& n)
{
    int base = str[0] == '$' ? 16 : 10;
    int i = str[0] == '$' ? 1 : 0;
    n = 0;

    for (; str[i] != 0; ++i)
    {
        int c = int(str[i]);

        n *= base;
        if (c >= '0' && c <= '9')
        {
            n += (c - '0');
        }
        else if (base == 16 && (c >= 'A' && c <= 'F'))
        {
            n += (c - 'A' + 10);
        }
        else if (base == 16 && (c >= 'a' && c <= 'f'))
        {
            n += (c - 'a' + 10);
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool Debugger::parseWord(const string& str, u16& out)
{
    int n;
    bool result = parseNumber(str, n) && (n >= 0 && n < 65536);
    if (result) out = u16(n);
    return result;
}

bool Debugger::parseByte(const string& str, u8& out)
{
    int n;
    bool result = parseNumber(str, n) && (n >= 0 && n < 256);
    if (result) out = u8(n);
    return result;
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
    m_disassemblyWindow.draw(draw);
    m_cpuStatusWindow.draw(draw);
    m_commandWindow.draw(draw);
}

//----------------------------------------------------------------------------------------------------------------------
// Commands
//----------------------------------------------------------------------------------------------------------------------

const vector<string>& Debugger::commands() const
{
    return m_memoryDumpWindow.isSelected() ? m_memoryDumpCommands : m_disassemblyCommands;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
