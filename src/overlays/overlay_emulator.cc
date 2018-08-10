//----------------------------------------------------------------------------------------------------------------------
//! Implements the EmulatorOverlay class
//!
//! @author     Matt Davies
//----------------------------------------------------------------------------------------------------------------------

#include <overlays/overlay_emulator.h>
#include <audio/audio.h>
#include <emulator/nx.h>
#include <emulator/spectrum.h>
#include <ui/draw.h>
#include <utils/format.h>
#include <utils/tinyfiledialogs.h>

//----------------------------------------------------------------------------------------------------------------------

EmulatorOverlay::EmulatorOverlay(Nx& nx)
    : Overlay(nx)
{
    clearKeys();
    calculateKeys();
    setMenu("Select Model", { "Spectrum 48K", "Spectrum 128K", "Spectrum +2 128K" },
        [this](int index) {
            static const Model models[] = {
                Model::ZX48,
                Model::ZX128,
                Model::ZXPlus2,
            };
            getSpeccy().apply(models[index]);
            getEmulator().rebuildLayers();
        });
}

//----------------------------------------------------------------------------------------------------------------------

void EmulatorOverlay::apply(const FrameState& frameState)
{
    Overlay::apply(frameState);
}

//----------------------------------------------------------------------------------------------------------------------

void EmulatorOverlay::onRender(Draw& draw)
{
    draw.wipeRect(0, 0, draw.getWidth(), draw.getHeight());
}

//----------------------------------------------------------------------------------------------------------------------

bool EmulatorOverlay::onKey(const KeyEvent& kev)
{
    SpeccyKey key1 = SpeccyKey::COUNT;
    SpeccyKey key2 = SpeccyKey::COUNT;

    using K = sf::Keyboard;

//     if (m_modelWindow.visible())
//     {
//         m_modelWindow.keyPress(key, down, shift, ctrl, alt);
//     }
//     else
    if (kev.isCtrl())
    {
        clearKeys();

        switch (kev.key)
        {
//         case K::K:
//             getEmulator().setSetting("kempston", getEmulator().getSetting("kempston") == "yes" ? "no" : "yes");
//             getEmulator().updateSettings();
//             showStatus();
//             break;

        case K::O:
            openFile({});
            break;

//         case K::S:
//             saveFile();
//             break;

//         case K::T:
//             getEmulator().showTapeBrowser();
//             break;

//         case K::A:
//             getEmulator().showEditor();
//             break;

//         case K::D:
//             getEmulator().showDisassembler();
//             break;

//         case K::Space:
//             if (getSpeccy().getTape())
//             {
//                 getSpeccy().getTape()->toggle();
//             }
//             break;

//         case K::Z:
//             getEmulator().toggleZoom();

        default:
            break;
        }
    }
    else
    {
        switch (kev.key)
        {
            //
            // Numbers
            //
        case K::Num1:       key1 = SpeccyKey::_1;         break;
        case K::Num2:       key1 = SpeccyKey::_2;         break;
        case K::Num3:       key1 = SpeccyKey::_3;         break;
        case K::Num4:       key1 = SpeccyKey::_4;         break;
        case K::Num5:       key1 = SpeccyKey::_5;         break;
        case K::Num6:       key1 = SpeccyKey::_6;         break;
        case K::Num7:       key1 = SpeccyKey::_7;         break;
        case K::Num8:       key1 = SpeccyKey::_8;         break;
        case K::Num9:       key1 = SpeccyKey::_9;         break;
        case K::Num0:       key1 = SpeccyKey::_0;         break;

            //
            // Letters
            //
        case K::A:          key1 = SpeccyKey::A;          break;
        case K::B:          key1 = SpeccyKey::B;          break;
        case K::C:          key1 = SpeccyKey::C;          break;
        case K::D:          key1 = SpeccyKey::D;          break;
        case K::E:          key1 = SpeccyKey::E;          break;
        case K::F:          key1 = SpeccyKey::F;          break;
        case K::G:          key1 = SpeccyKey::G;          break;
        case K::H:          key1 = SpeccyKey::H;          break;
        case K::I:          key1 = SpeccyKey::I;          break;
        case K::J:          key1 = SpeccyKey::J;          break;
        case K::K:          key1 = SpeccyKey::K;          break;
        case K::L:          key1 = SpeccyKey::L;          break;
        case K::M:          key1 = SpeccyKey::M;          break;
        case K::N:          key1 = SpeccyKey::N;          break;
        case K::O:          key1 = SpeccyKey::O;          break;
        case K::P:          key1 = SpeccyKey::P;          break;
        case K::Q:          key1 = SpeccyKey::Q;          break;
        case K::R:          key1 = SpeccyKey::R;          break;
        case K::S:          key1 = SpeccyKey::S;          break;
        case K::T:          key1 = SpeccyKey::T;          break;
        case K::U:          key1 = SpeccyKey::U;          break;
        case K::V:          key1 = SpeccyKey::V;          break;
        case K::W:          key1 = SpeccyKey::W;          break;
        case K::X:          key1 = SpeccyKey::X;          break;
        case K::Y:          key1 = SpeccyKey::Y;          break;
        case K::Z:          key1 = SpeccyKey::Z;          break;

            //
            // Other keys on the Speccy
            //
        case K::LShift:     key1 = SpeccyKey::Shift;      break;
        case K::RShift:     key1 = SpeccyKey::SymShift;   break;
        case K::Return:     key1 = SpeccyKey::Enter;      break;
        case K::Space:      key1 = SpeccyKey::Space;      break;

            //
            // Map PC keys to various keys on the Speccy
            //
        case K::BackSpace:  key1 = SpeccyKey::Shift;      key2 = SpeccyKey::_0;     break;
        case K::Escape:     key1 = SpeccyKey::Shift;      key2 = SpeccyKey::Space;  break;

        case K::SemiColon:
            key1 = SpeccyKey::SymShift;
            key2 = kev.shift ? SpeccyKey::Z : SpeccyKey::O;
            break;

        case K::Comma:
            key1 = SpeccyKey::SymShift;
            key2 = kev.shift ? SpeccyKey::R : SpeccyKey::N;
            break;

        case K::Period:
            key1 = SpeccyKey::SymShift;
            key2 = kev.shift ? SpeccyKey::T : SpeccyKey::M;
            break;

        case K::Quote:
            key1 = SpeccyKey::SymShift;
            key2 = kev.shift ? SpeccyKey::P : SpeccyKey::_7;
            break;

        case K::Slash:
            key1 = SpeccyKey::SymShift;
            key2 = kev.shift ? SpeccyKey::C : SpeccyKey::V;
            break;

        case K::Dash:
            key1 = SpeccyKey::SymShift;
            key2 = kev.shift ? SpeccyKey::_0 : SpeccyKey::J;
            break;

        case K::Equal:
            key1 = SpeccyKey::SymShift;
            key2 = kev.shift ? SpeccyKey::K : SpeccyKey::L;
            break;

        case K::Left:
//             if (getEmulator().usesKempstonJoystick())
//             {
//                 joystickKey(Joystick::Left, down);
//             }
//             else
//             {
                key1 = SpeccyKey::Shift;
                key2 = SpeccyKey::_5;
//             }
            break;

        case K::Down:
//             if (getEmulator().usesKempstonJoystick())
//             {
//                 joystickKey(Joystick::Down, down);
//             }
//             else
//             {
                key1 = SpeccyKey::Shift;
                key2 = SpeccyKey::_6;
//             }
            break;

        case K::Up:
//             if (getEmulator().usesKempstonJoystick())
//             {
//                 joystickKey(Joystick::Up, down);
//             }
//             else
//             {
                key1 = SpeccyKey::Shift;
                key2 = SpeccyKey::_7;
//             }
            break;

        case K::Right:
//             if (getEmulator().usesKempstonJoystick())
//             {
//                 joystickKey(Joystick::Right, down);
//             }
//             else
//             {
                key1 = SpeccyKey::Shift;
                key2 = SpeccyKey::_8;
//             }
            break;

        case K::Tab:
//             if (getEmulator().usesKempstonJoystick())
//             {
//                 joystickKey(Joystick::Fire, down);
//             }
//             else
//             {
                key1 = SpeccyKey::Shift;
                key2 = SpeccyKey::SymShift;
//             }
            break;

        case K::Tilde:
            //if (down) getEmulator().toggleDebugger();
            break;

        case K::F5:
            //if (down) getEmulator().togglePause(false);
            break;

        default:
            // If releasing a non-speccy key, clear all key map.
            clearKeys();
        }
    }

    if (key1 != SpeccyKey::COUNT)
    {
        m_speccyKeys[(int)key1] = kev.down;
    }
    if (key2 != SpeccyKey::COUNT)
    {
        m_speccyKeys[(int)key2] = kev.down;
    }

    // Fix for Windows crappy keyboard support.  It's not perfect but better than not dealing with it.
#ifdef _WIN32
    if ((kev.key == K::LShift || kev.key == K::RShift) && !kev.down)
    {
        m_speccyKeys[(int)SpeccyKey::Shift] = false;
        m_speccyKeys[(int)SpeccyKey::SymShift] = false;
    }
#endif

    calculateKeys();
    return true;
}

//----------------------------------------------------------------------------------------------------------------------

void EmulatorOverlay::onText(char ch)
{

}

//----------------------------------------------------------------------------------------------------------------------

void EmulatorOverlay::calculateKeys()
{
    vector<u8> keyRows(8, 0);
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
        keyRows[i] = keys;
    }

    getSpeccy().applyKeyboard(move(keyRows));
}

//----------------------------------------------------------------------------------------------------------------------

void EmulatorOverlay::clearKeys()
{
    fill(m_speccyKeys, m_speccyKeys + (int)SpeccyKey::COUNT, false);
    calculateKeys();
}

//----------------------------------------------------------------------------------------------------------------------

void EmulatorOverlay::openFile(string fileName)
{
    bool mute = getSpeccy().getAudio()->isMute();
    getSpeccy().getAudio()->mute(true);

    if (fileName.empty())
    {
        const char* filters[] = { "*.nx", "*.sna", "*.z80" };
        const char* fname = tinyfd_openFileDialog("Open File", 0, sizeof(filters) / sizeof(filters[0]), filters,
            "Nx Files", 0);
        if (fname) fileName = fname;
    }

    if (!fileName.empty())
    {
        if (!getEmulator().openFile(fileName))
        {
            error(stringFormat("Unable to load '{0}'", fileName));
        }
    }

    getSpeccy().getAudio()->mute(mute);
    getSpeccy().renderVRAM();
}

//----------------------------------------------------------------------------------------------------------------------
