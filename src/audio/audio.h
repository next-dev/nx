//----------------------------------------------------------------------------------------------------------------------
// Audio System
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <core.h>

#include <functional>
#include <mutex>
#include <portaudio/portaudio.h>

#define NX_AUDIO_SAMPLERATE 44100
#define NX_DISABLE_AUDIO    0

//----------------------------------------------------------------------------------------------------------------------
// Signals
//----------------------------------------------------------------------------------------------------------------------

class Signal
{
public:
    Signal()
        : m_triggered(false)
    {

    }

    // Trigger a signal from remote thread
    void trigger()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_triggered = true;
    }

    // Will reset the signal state when checked if triggered.
    bool isTriggered()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool result = m_triggered;
        m_triggered = false;
        return result;
    }

private:
    std::mutex      m_mutex;
    bool            m_triggered;
};

//----------------------------------------------------------------------------------------------------------------------
// Audio system
//----------------------------------------------------------------------------------------------------------------------

class Audio
{
public:
    Audio(int numTStatesPerFrame);
    ~Audio();

    void start();
    void stop();

    void updateBeeper(i64 tState, u8 speaker, u8 tape);
    void mute(bool enabled) { m_mute = enabled; }

    bool isMute() const { return m_mute; }

    Signal& getSignal() { return m_renderSignal; }

private:
    void initialiseBuffers();

    static int callback(const void* input,
        void* output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);

private:
    int                 m_numTStatesPerSample;
    int                 m_numSamplesPerFrame;
    int                 m_numTStatesPerFrame;
    int                 m_sampleRate;
    i16*                m_soundBuffer;
    i16*                m_playBuffer;
    i16*                m_fillBuffer;
    i64                 m_tStatesUpdated;
    i64                 m_tStateCounter;
    int                 m_audioValue;
    int                 m_tapeAudioValue;
    int                 m_writePosition;

    PaHostApiIndex      m_audioHost;
    PaDeviceIndex       m_audioDevice;
    PaStream*           m_stream;

    Signal              m_renderSignal;

    bool                m_mute;
    bool                m_started;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
