//----------------------------------------------------------------------------------------------------------------------
// Nx class
//----------------------------------------------------------------------------------------------------------------------

#include "nx.h"

#ifdef __APPLE__
#   include "ResourcePath.hpp"
#endif

#include <algorithm>

// Scaling constants for the Speccy pixel to PC pixel ratio.
static const int kScale = 4;
static const int kUiScale = 2;

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Nx::Nx()
    : m_machine(nullptr)
    , m_speccyKeys((int)Key::COUNT)
    , m_keyRows(8)
    , m_debugger(false)
    , m_window(sf::VideoMode(kWindowWidth * kScale, kWindowHeight * kScale), "NX " NX_VERSION,
               sf::Style::Titlebar | sf::Style::Close)
{
    m_machine = new Spectrum();
    sf::FileInputStream f;
#ifdef __APPLE__
    string romFileName = resourcePath() + "48.rom";
#else
    string romFileName = "48.rom";
#endif
    if (f.open(romFileName))
    {
        i64 size = min(f.getSize(), (i64)16384);
        vector<u8> buffer(size);
        f.read(buffer.data(), size);

        m_machine->load(0, buffer.data(), size);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Destruction
//----------------------------------------------------------------------------------------------------------------------

Nx::~Nx()
{
    delete[] m_machine;
}

//----------------------------------------------------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------------------------------------------------

void Nx::render()
{
    m_window.clear();
    m_window.draw(m_machine->getVideoSprite());
    m_window.display();
}

//----------------------------------------------------------------------------------------------------------------------
// Running
//----------------------------------------------------------------------------------------------------------------------

void Nx::run()
{
    while (m_window.isOpen())
    {
        sf::Event event;

        sf::Clock clk;

        //
        // Process the OS events
        //
        while (m_window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                m_window.close();
                break;

            case sf::Event::KeyPressed:
                spectrumKey(event.key.code, true);
                break;

            case sf::Event::KeyReleased:
                spectrumKey(event.key.code, false);
                break;

            default:
                break;
            }
        }

        //
        // Generate a frame
        //
        m_machine->setKeyboardState(m_keyRows);
        frame();
        render();

        //
        // Synchronise with real time
        //
        sf::Time elapsedTime = clk.restart();
        sf::Time timeLeft = sf::milliseconds(20) - elapsedTime;
        sf::sleep(timeLeft);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Frame generation
//----------------------------------------------------------------------------------------------------------------------

void Nx::frame()
{
    m_machine->update();
}

//----------------------------------------------------------------------------------------------------------------------
// Keyboard
//----------------------------------------------------------------------------------------------------------------------

void Nx::spectrumKey(sf::Keyboard::Key key, bool down)
{
    Key key1 = Key::COUNT;
    Key key2 = Key::COUNT;
    
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
            key1 = Key::Shift;
            key2 = Key::_5;
            break;
            
        case K::Down:
            key1 = Key::Shift;
            key2 = Key::_6;
            break;
            
        case K::Up:
            key1 = Key::Shift;
            key2 = Key::_7;
            break;
            
        case K::Right:
            key1 = Key::Shift;
            key2 = Key::_8;
            break;
            
        case K::Tab:
            key1 = Key::Shift;
            key2 = Key::SymShift;
            break;
            
        case K::Tilde:
            if (down) m_debugger = !m_debugger;
            break;
            
        case K::F2:
            m_machine->reset(false);
            break;
            
        default:
            // If releasing a non-speccy key, clear all key map.
            fill(m_speccyKeys.begin(), m_speccyKeys.end(), false);
    }
    
    if (key1 != Key::COUNT)
    {
        m_speccyKeys[(int)key1] = down;
    }
    if (key2 != Key::COUNT)
    {
        m_speccyKeys[(int)key2] = down;
    }
    
    // Fix for Windows crappy keyboard support.  It's not perfect but better than not dealing with it.
#ifdef _WIN32
    if ((key == K::LShift || key == K::RShift) && !down)
    {
        N.keyPress(Key::Shift, false);
        N.keyPress(Key::SymShift, false);
    }
#endif

    calculateKeys();
}

void Nx::debuggerKey(sf::Keyboard::Key key)
{
    
}

void Nx::calculateKeys()
{
    for (int i = 0; i < 8; ++i)
    {
        u8 keys = 0;
        u8 key = 1;
        for (int j = 0; j < 5; ++j)
        {
            if (m_speccyKeys[i * 5 + j])
            {
                keys += key;
            }
            key <<= 1;
        }
        m_keyRows[i] = keys;
    }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------














































