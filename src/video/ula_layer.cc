//----------------------------------------------------------------------------------------------------------------------
//! Implements the UlaLayer class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <video/ula_layer.h>
#include <utils/format.h>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// U L A L A Y E R
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

UlaLayer::UlaLayer(const Memory& memory)
    : m_memory(memory)
    , m_state()
    , m_videoMap()
    , m_videoWrite(0)
    , m_startTState(0)
    , m_drawTState(0)
{
    recalcVideoMaps();
}

//----------------------------------------------------------------------------------------------------------------------

UlaLayer::~UlaLayer()
{

}

//----------------------------------------------------------------------------------------------------------------------

void UlaLayer::apply(const VideoState& state)
{
    bool recalcMaps = false;
    if (m_state.videoBank != state.videoBank)
    {
        recalcMaps = true;
    }
    m_state = state;
    if (recalcMaps)
    {
        // #speed: Figure out a way to speed this up (cache video maps)?
        recalcVideoMaps();
    }
}

//----------------------------------------------------------------------------------------------------------------------

static const u16 kDoNotDraw = 0xffff;
static const u16 kBorder = 0xfffe;

void UlaLayer::update()
{
    static const u32 colours[16] =
    {
        0xff000000, 0xffd70000, 0xff0000d7, 0xffd700d7, 0xff00d700, 0xffd7d700, 0xff00d7d7, 0xffd7d7d7,
        0xff000000, 0xffff0000, 0xff0000ff, 0xffff00ff, 0xff00ff00, 0xffffff00, 0xff00ffff, 0xffffffff,
    };

    // Nothing to draw yet
//     m_state.tStates = kFrameTStates - 1;
//     m_drawTState = m_startTState;
//     m_videoWrite = 0;
// 
    TState tState = m_state.tStates;
    if (tState < m_startTState) return;
    if (tState >= kFrameTStates)
    {
        tState = kFrameTStates - 1;
    }

    // It takes 4 t-states to write 1 byte.
    int elapsedTStates = int(tState + 1 - m_drawTState);
    int numBytes = (elapsedTStates >> 2) + ((elapsedTStates % 4) > 0 ? 1 : 0);

    for (int i = 0; i < numBytes; ++i)
    {
        if (m_videoMap[m_drawTState] < kBorder)
        {
            // Pixel address
            // #todo: refactor bank sizes to log-2?
            u16 paddr = m_videoMap[m_drawTState];
            u16 bank = m_state.videoBank.index() + (paddr / kBankSize);
            u16 offset = paddr % kBankSize;
            u8 pixelData = m_memory.peek8(MemAddr(Bank(MemGroup::RAM, bank), offset));

            // Calculate attribute address
            // 000S SRRR CCCX XXXX --> 0001 10SS CCCX XXXX
            u16 aaddr = ((paddr & 0x1800) >> 3) + (paddr & 0x00ff) + 0x1800;
            bank = m_state.videoBank.index() + (aaddr / kBankSize);
            offset = aaddr % kBankSize;
            u8 attr = m_memory.peek8(MemAddr(Bank(MemGroup::RAM, bank), offset));

            // Bright is either 0x08 or 0x00
            u8 bright = (attr & 0x40) >> 3;
            u8 ink = (attr & 0x07);
            u8 paper = (attr & 0x38) >> 3;
            u32 c0 = colours[paper + bright];
            u32 c1 = colours[ink + bright];

            if (m_state.flash && (attr & 0x80))
            {
                std::swap(c0, c1);
            }

            // We have a byte of pixel data and a byte of attr data - draw 8 pixels
            for (int p = 0; p < 8; ++p)
            {
                if ((pixelData & 0x80) != 0)
                {
                    NX_ASSERT(m_videoWrite < (kWindowWidth * kWindowHeight));
                    m_image[m_videoWrite++] = c1;
                }
                else
                {
                    NX_ASSERT(m_videoWrite < (kWindowWidth * kWindowHeight));
                    m_image[m_videoWrite++] = c0;
                }
                pixelData <<= 1;
            }
        } // pixel data
        else if (m_videoMap[m_drawTState] == kBorder)
        {
            u32 border = colours[m_state.borderColour];
            for (int b = 0; b < 8; ++b)
            {
                NX_ASSERT(m_videoWrite < (kWindowWidth * kWindowHeight));
                m_image[m_videoWrite++] = border;
            }
        }

        m_drawTState += 4;
    } // for numBytes

    if (m_state.tStates >= kFrameTStates)
    {
        m_videoWrite = 0;
        m_drawTState = m_startTState;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void UlaLayer::render()
{
}

//----------------------------------------------------------------------------------------------------------------------

// This needs to be called every time the video bank changes
void UlaLayer::recalcVideoMaps()
{
    m_videoMap.resize(kFrameTStates);

    // Start of display area is 14336.  We wait 4 t-states before we draw 8 pixels.  The left border is 24 pixels wide.
    // Each scan line is 224 t-states long
    m_startTState = (14340 - 24) - (224 * kBorderHeight);

    // Initialise the t-state->address map
    //
    // Values will be:
    //      0xffff  Do not draw
    //      0xfffe  Border colour
    //      0x0000+ Pixel address
    //
    int t = 0;

    // Vertical retrace period (and top border outside window)
    for (t = 0; t < m_startTState; ++t)
    {
        m_videoMap[t] = kDoNotDraw;
    }

    // Calculate line timings
    //
    // +---------- TV width ------------------+
    // |   +------ Window width ----------+   |
    // |   |  +--- Screen width -------+  |   |
    // v   v  v                        v  v   v
    // +---+--+------------------------+--+---+-----+
    // |000|11|aaaaaaaaaaaaaaaaaaaaaaaa|11|000|00000|
    // +---+--+------------------------+--+---+-----+
    //     ta tb                          176-ta    224
    //                                 176-tb
    int ta = (kTvWidth - kWindowWidth) / 4;
    int tb = (kTvWidth - kScreenWidth) / 4;

    // Top border
    while (t < (m_startTState + (kBorderHeight * 224)))
    {
        // The line is: border (w/2t), screen (128t), border (w/2t) = 128+wt
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = kDoNotDraw;
        for (int i = ta; i < 176 - ta; ++i) m_videoMap[t++] = kBorder;
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = kDoNotDraw;
    }

    // Build middle of display
    int x = 0, y = 0;
    int addr = 2;

    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224))
    {
        // Left border
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = kDoNotDraw;
        for (int i = ta; i < tb; ++i) m_videoMap[t++] = kBorder;

        // Screen line
        for (int i = tb; i < tb + 128; ++i)
        {
            // Every 4 t-states (8 pixels), we recalculate the address
            if (i % 4 == 0)
            {
                // Pixel offset is 000S SRRR CCCX XXXX, where Y = SSCCCRRR
                // Attr offset is  0001 10YY YYYX XXXX
                addr =
                    // Hi byte
                    // SS-------------+   RRR------+   
                    ((((y & 0xc0) >> 3) | (y & 0x07)) << 8) |       // ---SSRRR --------
                    // Lo byte
                    // XXXXX---------+   CCC-------------+
                    (((x >> 3) & 0x1f) | ((y & 0x38) << 2));        // -------- CCCXXXXX
                x += 8;
            }
            m_videoMap[t++] = addr;
        }
        ++y;

        // Right border
        for (int i = tb + 128; i < 176 - ta; ++i) m_videoMap[t++] = kBorder;

        // Horizontal retrace + out of screen border
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = kDoNotDraw;
    }

    // Bottom border
    while (t < m_startTState + (kBorderHeight * 224) + (kScreenHeight * 224) + (kBorderHeight * 224))
    {
        for (int i = 0; i < ta; ++i) m_videoMap[t++] = kDoNotDraw;
        for (int i = ta; i < 176 - ta; ++i) m_videoMap[t++] = kBorder;
        for (int i = 176 - ta; i < 224; ++i) m_videoMap[t++] = kDoNotDraw;
    }

    while (t < kFrameTStates)
    {
        m_videoMap[t++] = kDoNotDraw;
    }
}

