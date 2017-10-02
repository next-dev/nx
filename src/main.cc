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

    void clear() override
    {
        m_redraw = false;
    }

    void redraw() override
    {
        m_redraw = true;
    }

    //
    // Querying
    //
    bool getRedraw() const { return m_redraw; }

private:
    HandleManager<std::vector<u8>> m_handles;
    bool m_redraw;
};


int main()
{
    Host host;
    sf::RenderWindow window(sf::VideoMode(kWindowWidth * 4, kWindowHeight * 4), "NX " NX_VERSION);
    sf::Texture tex;
    tex.create(kWindowWidth, kWindowHeight);
    sf::Sprite sprite(tex);
    sprite.setScale(4, 4);

    u32* img = new u32[kWindowWidth * kWindowHeight];

    Nx nx(host, img);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        nx.update();

        if (host.getRedraw())
        {
            window.clear();
            tex.update((const sf::Uint8 *)img);
            window.draw(sprite);
            window.display();
        }
    }

    delete[] img;

    return 0;
}
