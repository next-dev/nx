//----------------------------------------------------------------------------------------------------------------------
// Nx class
//----------------------------------------------------------------------------------------------------------------------

#include "nx.h"

#include <algorithm>

// Scaling constants for the Speccy pixel to PC pixel ratio.
static const int kScale = 4;
static const int kUiScale = 2;

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

Nx::Nx()
    : m_machine(nullptr)
    , m_window(sf::VideoMode(kWindowWidth * kScale, kWindowHeight * kScale), "NX " NX_VERSION,
               sf::Style::Titlebar | sf::Style::Close)
{
    m_machine = new Spectrum();
    sf::FileInputStream f;
    if (f.open("48.rom"))
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
                break;

            case sf::Event::KeyReleased:
                break;

            default:
                break;
            }
        }

        //
        // Generate a frame
        //
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
//----------------------------------------------------------------------------------------------------------------------
