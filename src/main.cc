//----------------------------------------------------------------------------------------------------------------------
// NX - Next Emulator
//----------------------------------------------------------------------------------------------------------------------

#include <emulator/nx.h>

//----------------------------------------------------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// Application class
//----------------------------------------------------------------------------------------------------------------------

class Application
{
public:
    Application(int argc, char** argv);

    void run();
    static void console();

private:
    Nx  m_emulator;
};

//----------------------------------------------------------------------------------------------------------------------
// Construction
//----------------------------------------------------------------------------------------------------------------------

Application::Application(int argc, char** argv)
    : m_emulator(argc, argv)
{
}

//----------------------------------------------------------------------------------------------------------------------
// Console
//----------------------------------------------------------------------------------------------------------------------

#if defined(_WIN32) && NX_DEBUG_CONSOLE

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>

void Application::console()
{
    AllocConsole();
    SetConsoleTitleA("Debug Window");

    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((intptr_t)handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    freopen("CONOUT$", "w", stdout);

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((intptr_t)handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 0);
    freopen("CONIN$", "r", stdin);

    DWORD mode;
    GetConsoleMode(handle_out, &mode);
    mode |= 0x4;
    SetConsoleMode(handle_out, mode);
}

#else

void Application::console()
{
    // Do nothing!
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// Running
//----------------------------------------------------------------------------------------------------------------------

void Application::run()
{
    m_emulator.run();
}

//----------------------------------------------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    Application::console();
    Application app(argc, argv);
    app.run();
}
