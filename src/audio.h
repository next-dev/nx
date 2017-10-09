//----------------------------------------------------------------------------------------------------------------------
// Audio System
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <portaudio/portaudio.h>
#include <atomic>

#define NX_AUDIO_SAMPLERATE 44100

//----------------------------------------------------------------------------------------------------------------------
// Audio system
//----------------------------------------------------------------------------------------------------------------------

class Audio
{
public:
    Audio(int numTStatesPerFrame);
    ~Audio();

    int numTStatesPerSample() const { return m_numTStatesPerFrame / numSamplesPerFrame(); }
    int numSamplesPerFrame() const { return m_sampleRate / 50; }
    
    void waitFrame();
    
private:
    void initialiseBuffers();
    
    static int callback(const void* input,
                        void* output,
                        unsigned long frameCount,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void* userData);
    
private:
    int             m_numTStatesPerFrame;
    int             m_sampleRate;
    i16*            m_soundBuffer;
    i16*            m_playBuffer;
    i16*            m_fillBuffer;
    
    PaHostApiIndex  m_audioHost;
    PaDeviceIndex   m_audioDevice;
    PaStream*       m_stream;
    
    std::atomic<bool>   m_frame;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

//----------------------------------------------------------------------------------------------------------------------
// Audio
//----------------------------------------------------------------------------------------------------------------------

Audio::Audio(int numTStatesPerFrame)
    : m_numTStatesPerFrame(numTStatesPerFrame)
    , m_soundBuffer(nullptr)
    , m_playBuffer(nullptr)
    , m_fillBuffer(nullptr)

{
    Pa_Initialize();
    m_audioHost = Pa_GetDefaultHostApi();
    m_audioDevice = Pa_GetDefaultOutputDevice();
    
    // Output information about the audio system
    const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(m_audioHost);
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(m_audioDevice);
    m_sampleRate = deviceInfo->defaultSampleRate;
    
    printf("Audio host: %s\n", hostInfo->name);
    printf("Audio device: %s\n", deviceInfo->name);
    printf("        rate: %g\n", deviceInfo->defaultSampleRate);
    printf("     latency: %g\n", deviceInfo->defaultLowOutputLatency);

    // We know the sample rate now, so let's initialise our buffers.
    initialiseBuffers();

    // Let's set up continuous streaming.
    Pa_OpenDefaultStream(&m_stream,
                         0,
                         1,
                         paInt16,
                         (double)m_sampleRate,
                         numSamplesPerFrame(),
                         &Audio::callback,
                         this);
    Pa_StartStream(m_stream);
}

Audio::~Audio()
{
    Pa_StopStream(m_stream);
    Pa_Terminate();
    delete [] m_soundBuffer;
}

void Audio::initialiseBuffers()
{
    // Each buffer needs to hold enough samples for a frame.  We'll double-buffer it so that one is the play buffer
    // and the other is the fill buffer.
    if (m_soundBuffer) delete[] m_soundBuffer;
    
    int numSamples = numSamplesPerFrame() * 2;
    m_soundBuffer = new i16[numSamples];
    m_playBuffer = m_soundBuffer;
    m_fillBuffer = m_soundBuffer + numSamplesPerFrame();
    
    int bufferSizeInSamples = numSamplesPerFrame();
    for (int i = 0; i < bufferSizeInSamples * 2; ++i) m_soundBuffer[i] = (i % 9) < 4 ? -10000 : 10000;
}

int Audio::callback(const void *input,
                    void *output,
                    unsigned long frameCount,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *userData)
{
    Audio* self = (Audio *)userData;
    self->m_frame = true;
    i16* outputBuffer = (i16 *)output;
    memcpy(outputBuffer, self->m_playBuffer, frameCount * sizeof(i16));
    return paContinue;
}

void Audio::waitFrame()
{
    m_frame = false;
    while (!m_frame) ;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
