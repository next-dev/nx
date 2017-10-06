//----------------------------------------------------------------------------------------------------------------------
// NX - Next Emulator
//----------------------------------------------------------------------------------------------------------------------

#define NX_DEBUG_CONSOLE    0

#include <SFML/Graphics.hpp>

#include <cstdint>

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

#define NX_MINOR_VERSION        0
#define NX_MAJOR_VERSION        0
#define NX_PATCH_VERSION        1

#define NX_STR2(x) #x
#define NX_STR(x) NX_STR2(x)

#if NX_MAJOR_VERSION == 0
#   if NX_MINOR_VERSION == 0
#       define NX_VERSION "Dev." NX_STR(NX_PATCH_VERSION)
#   elif NX_MINOR_VERSION == 9
#       define NX_VERSION "Beta." NX_STR(NX_PATCH_VERSION)
#   else
#       define NX_VERSION "Alpha." NX_STR(NX_PATCH_VERSION)
#   endif
#else
#   define NX_VERSION NX_STR(NX_MAJOR_VERSION) "." NX_STR(NX_MINOR_VERSION) "." NX_STR(NX_PATCH_VERSION)
#endif

#define NX_IMPL
#include "host.h"
#include "nx.h"

//----------------------------------------------------------------------------------------------------------------------
// Handles
//----------------------------------------------------------------------------------------------------------------------

template <typename T>
class HandleManager
{
public:
    int add(T&& t)
    {
        int h = m_handles.size() == 0 ? 1 : m_handles.back().handle;
        m_handles.emplace_back( HandleInfo { h, std::forward<T>(t) });
        return h;
    }

    void remove(int handle)
    {
        auto it = std::find_if(m_handles.begin(), m_handles.end(),
            [handle](const HandleInfo& h1) -> bool
            {
                return h1.handle == handle;
            });
        if (it != m_handles.end()) m_handles.erase(it);
    }

    T& get(int handle)
    {
        static T dummy;

        auto it = std::find_if(m_handles.begin(), m_handles.end(),
            [handle](const HandleInfo& h1)
        {
            h1.handle == handle;
        });
        return (it != m_handles.end()) ? it->t : dummy;
    }

private:
    struct HandleInfo
    {
        int handle;
        T t;
    };

    std::vector<HandleInfo> m_handles;
};

//----------------------------------------------------------------------------------------------------------------------
// Host interface
//----------------------------------------------------------------------------------------------------------------------

class Host : public IHost
{
public:
    //
    // IHost interface
    //
    int load(std::string fileName, const u8*& outBuffer, i64& outSize) override
    {
        sf::FileInputStream f;
        if (f.open(fileName))
        {
            std::vector<u8> buffer(f.getSize());
            outBuffer = buffer.data();
            outSize = (i64)buffer.size();
            f.read(buffer.data(), outSize);
            return m_handles.add(std::move(buffer));
        }

        return 0;
    }

    void unload(int handle) override
    {
        m_handles.remove(handle);
    }

    //
    // Querying
    //
    bool getRedraw() const { return m_redraw; }

private:
    HandleManager<std::vector<u8>> m_handles;
    bool m_redraw;
};

//----------------------------------------------------------------------------------------------------------------------
// File open dialog
//----------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32

typedef struct
{
    const char*   title;
    const char*   path;
    const char*   filterName;
    const char*   filter;
}
WindowFileOpenConfig;

const char* windowFileOpen(WindowFileOpenConfig* config);
const char* windowFileSaveAs(WindowFileOpenConfig* config);

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commdlg.h>
#include <string.h>

static int win32NextFilter(char* dst, const char** p)
{
    int len;

    *p += strspn(*p, "|");
    if (**p == '\0') {
        return 0;
    }

    len = (int)strcspn(*p, "|");
    memcpy(dst, *p, len);
    dst[len] = '\0';
    *p += len;

    return 1;
}

static const char* win32MakeFilterString(WindowFileOpenConfig* config)
{
    static char buf[1024];
    int n = 0;

    buf[0] = 0;

    if (config->filter)
    {
        const char* p;
        char b[32];
        const char* name = config->filterName ? config->filterName : config->filter;

        n += sprintf(buf + n, "%s", name) + 1;

        p = config->filter;
        while (win32NextFilter(b, &p))
        {
            n += sprintf(buf + n, "%s;", b);
        }

        buf[++n] = 0;
    }

    n += sprintf(buf + n, "All Files") + 1;
    n += sprintf(buf + n, "*.*");
    buf[++n] = 0;

    return buf;
}

static void win32InitOpenFileName(WindowFileOpenConfig* config, OPENFILENAMEA* ofn)
{
    static char fileName[2048];

    fileName[0] = 0;
    memset(ofn, 0, sizeof(*ofn));
    ofn->lStructSize = sizeof(*ofn);
    ofn->lpstrFilter = win32MakeFilterString(config);
    ofn->nFilterIndex = 1;
    ofn->lpstrFile = fileName;
    ofn->Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn->nMaxFile = sizeof(fileName) - 1;
    ofn->lpstrInitialDir = config->path;
    ofn->lpstrTitle = config->title;
    ofn->lpstrTitle = config->title;
}

const char* windowFileOpen(WindowFileOpenConfig* config)
{
    OPENFILENAMEA ofn;
    win32InitOpenFileName(config, &ofn);
    return GetOpenFileNameA(&ofn) ? ofn.lpstrFile : 0;
}

const char* windowFileSaveAs(WindowFileOpenConfig* config)
{
    OPENFILENAMEA ofn;
    win32InitOpenFileName(config, &ofn);
    return GetSaveFileNameA(&ofn) ? ofn.lpstrFile : 0;
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// Keyboard handling
//----------------------------------------------------------------------------------------------------------------------

void debugKey(Nx& N, sf::Keyboard::Key key, bool down)
{
    using K = sf::Keyboard;

    UiKey k = UiKey::COUNT;

    switch (key)
    {
    case K::Left:       k = UiKey::Left;        break;
    case K::Down:       k = UiKey::Down;        break;
    case K::Up:         k = UiKey::Up;          break;
    case K::Right:      k = UiKey::Right;       break;
    case K::PageUp:     k = UiKey::PageUp;      break;
    case K::PageDown:   k = UiKey::PageDn;      break;
    case K::Tab:        k = UiKey::Tab;         break;

    case K::Tilde:      if (down) N.toggleDebugger();       break;
    case K::F5:         if (down) N.togglePause();          break;
            
    default: break;
    }

    if (k != UiKey::COUNT)
    {
        N.uiKeyPress(k, down);
    }
}

void key(Nx& N, sf::Keyboard::Key key, bool down)
{
    Key key1 = Key::COUNT;
    Key key2 = Key::COUNT;

    //printf("KEY [%s]: %d\n", down ? "DOWN" : " UP ", key);

    using K = sf::Keyboard;

    switch (key)
    {
    case K::Num1:       key1 = Key::_1;         break;
    case K::Num2:       key1 = Key::_2;         break;
    case K::Num3:       key1 = Key::_3;         break;
    case K::Num4:       key1 = Key::_4;         break;
    case K::Num5:       key1 = Key::_5;         break;
    case K::Num6:       key1 = Key::_6;         break;
    case K::Num7:       key1 = Key::_7;         break;
    case K::Num8:       key1 = Key::_8;         break;
    case K::Num9:       key1 = Key::_9;         break;
    case K::Num0:       key1 = Key::_0;         break;

    case K::A:          key1 = Key::A;          break;
    case K::B:          key1 = Key::B;          break;
    case K::C:          key1 = Key::C;          break;
    case K::D:          key1 = Key::D;          break;
    case K::E:          key1 = Key::E;          break;
    case K::F:          key1 = Key::F;          break;
    case K::G:          key1 = Key::G;          break;
    case K::H:          key1 = Key::H;          break;
    case K::I:          key1 = Key::I;          break;
    case K::J:          key1 = Key::J;          break;
    case K::K:          key1 = Key::K;          break;
    case K::L:          key1 = Key::L;          break;
    case K::M:          key1 = Key::M;          break;
    case K::N:          key1 = Key::N;          break;
    case K::O:          key1 = Key::O;          break;
    case K::P:          key1 = Key::P;          break;
    case K::Q:          key1 = Key::Q;          break;
    case K::R:          key1 = Key::R;          break;
    case K::S:          key1 = Key::S;          break;
    case K::T:          key1 = Key::T;          break;
    case K::U:          key1 = Key::U;          break;
    case K::V:          key1 = Key::V;          break;
    case K::W:          key1 = Key::W;          break;
    case K::X:          key1 = Key::X;          break;
    case K::Y:          key1 = Key::Y;          break;
    case K::Z:          key1 = Key::Z;          break;

    case K::LShift:     key1 = Key::Shift;      break;
    case K::RShift:     key1 = Key::SymShift;   break;
    case K::Return:     key1 = Key::Enter;      break;
    case K::Space:      key1 = Key::Space;      break;

    case K::BackSpace:  key1 = Key::Shift;      key2 = Key::_0;     break;
    case K::Escape:     key1 = Key::Shift;      key2 = Key::Space;  break;

    case K::Left:
        if (N.usesKempstonJoystick())
        {
            N.joystickKeyPress(JoystickKey::Left, down);
        }
        else
        {
            key1 = Key::Shift;
            key2 = Key::_5;
        }
        break;

    case K::Down:
        if (N.usesKempstonJoystick())
        {
            N.joystickKeyPress(JoystickKey::Down, down);
        }
        else
        {
            key1 = Key::Shift;
            key2 = Key::_6;
        }
        break;

    case K::Up:
        if (N.usesKempstonJoystick())
        {
            N.joystickKeyPress(JoystickKey::Up, down);
        }
        else
        {
            key1 = Key::Shift;
            key2 = Key::_7;
        }
        break;

    case K::Right:
        if (N.usesKempstonJoystick())
        {
            N.joystickKeyPress(JoystickKey::Right, down);
        }
        else
        {
            key1 = Key::Shift;
            key2 = Key::_8;
        }
        break;

    case K::Tab:
        if (N.usesKempstonJoystick()) N.joystickKeyPress(JoystickKey::Fire, down);
        break;

    case K::Tilde:      if (down) N.toggleDebugger();               break;
    case K::F5:         if (down) N.togglePause();                  break;

#ifdef _WIN32
    case K::F1:
        // File Open
        {
            N.keysClear();

            // Last path opened
            static char path[2048] = { 0 };

            // Open file
            WindowFileOpenConfig cfg = {
                "Open file",
                path,
                "NX files",
                "*.sna"
            };
            const char* fileName = windowFileOpen(&cfg);
            if (!fileName) break;

            // Store the path for next time
            const char* end = strrchr(fileName, '\\');
            strncpy(path, (const char *)fileName, (size_t)(path - end));

            if (!N.load(fileName))
            {
                MessageBoxA(0, "Unable to load!", "ERROR", MB_ICONERROR | MB_OK);
            }
        }
        break;
#endif

    case K::F12:
        N.restart();
        break;

    default:
        // If releasing a non-speccy key, clear all key map.
        N.keysClear();
    }

    if (key1 != Key::COUNT)
    {
        N.keyPress(key1, down);
    }
    if (key2 != Key::COUNT)
    {
        N.keyPress(key2, down);
    }

    if ((key == K::LShift || key == K::RShift) && !down)
    {
        N.keyPress(Key::Shift, false);
        N.keyPress(Key::SymShift, false);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Console
//----------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32

#include <conio.h>
#include <fcntl.h>
#include <io.h>

void console()
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

#endif

//----------------------------------------------------------------------------------------------------------------------
// Main loop
//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
#if NX_DEBUG_CONSOLE && defined(_WIN32)
    console();
#endif
    
    static int kScale = 4;

    Host host;

    // Spectrum video
    sf::RenderWindow window(sf::VideoMode(kWindowWidth * kScale, kWindowHeight * kScale), "NX " NX_VERSION,
        sf::Style::Titlebar | sf::Style::Close);
    sf::Texture tex;
    tex.create(kWindowWidth, kWindowHeight);
    sf::Sprite sprite(tex);
    sprite.setScale(kScale, kScale);
    u32* img = new u32[kWindowWidth * kWindowHeight];

    // UI video
    sf::Texture uitex;
    uitex.create(kUiWidth, kUiHeight);
    sf::Sprite uiSprite(uitex);
    uiSprite.setScale(kScale/2, kScale/2);
    u32* ui_img = new u32[kUiWidth * kUiHeight];

    Nx N(host, img, ui_img, argc, argv);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::KeyPressed:
                if (N.isDebugging())
                {
                    debugKey(N, event.key.code, true);
                }
                else
                {
                    key(N, event.key.code, true);
                }
                break;

            case sf::Event::KeyReleased:
                if (N.isDebugging())
                {
                    debugKey(N, event.key.code, false);
                }
                else
                {
                    key(N, event.key.code, false);
                }
                break;
                    
            default: break;
            }
        }

        sf::Clock clk;

        N.update();

        window.clear();
        tex.update((const sf::Uint8 *)img);
        uitex.update((const sf::Uint8 *)ui_img);
        window.draw(sprite);
        window.draw(uiSprite);
        window.display();

        sf::Time elapsedTime = clk.getElapsedTime();
        sf::Time timeLeft = sf::milliseconds(20) - elapsedTime;
        sf::sleep(timeLeft);
    }

    delete[] img;
    delete[] ui_img;

    return 0;
}
