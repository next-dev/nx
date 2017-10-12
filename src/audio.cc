//----------------------------------------------------------------------------------------------------------------------
// Audio implementation
//----------------------------------------------------------------------------------------------------------------------

#include "audio.h"

#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// Audio
//----------------------------------------------------------------------------------------------------------------------

Audio::Audio(int numTStatesPerFrame, function<void()> frameFunc)
    : m_numTStatesPerSample(0)
    , m_numSamplesPerFrame(0)
    , m_numTStatesPerFrame(numTStatesPerFrame)
    , m_soundBuffer(nullptr)
    , m_playBuffer(nullptr)
    , m_fillBuffer(nullptr)
    , m_tStatesUpdated(0)
    , m_tStateCounter(0)
    , m_audioValue(0)
    , m_writePosition(0)
    , m_audioHost(0)
    , m_audioDevice(0)
    , m_stream(nullptr)
    , m_frameFunc(frameFunc)
{
    Pa_Initialize();
    m_audioHost = Pa_GetDefaultHostApi();
    m_audioDevice = Pa_GetDefaultOutputDevice();

    // Output information about the audio system
    const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(m_audioHost);
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(m_audioDevice);
    m_sampleRate = (int)deviceInfo->defaultSampleRate;

    //m_numTStatesPerSample = numTStatesPerFrame / (m_sampleRate / 50);
    //m_numSamplesPerFrame = numTStatesPerFrame / m_numTStatesPerSample;

    m_numSamplesPerFrame = m_sampleRate / 50;
    m_numTStatesPerSample = numTStatesPerFrame / m_numSamplesPerFrame;

    printf("Audio host: %s\n", hostInfo->name);
    printf("Audio device: %s\n", deviceInfo->name);
    printf("        rate: %g\n", deviceInfo->defaultSampleRate);
    printf("     latency: %g\n", deviceInfo->defaultLowOutputLatency);

    // We know the sample rate now, so let's initialise our buffers.
    initialiseBuffers();

    // Let's set up continuous streaming.
    PaStreamParameters output;
    output.channelCount = 1;
    output.device = m_audioDevice;
    output.hostApiSpecificStreamInfo = nullptr;
    output.sampleFormat = paInt16;
    //output.suggestedLatency = 0;
    output.suggestedLatency = deviceInfo->defaultLowOutputLatency;

    Pa_OpenStream(&m_stream,
        nullptr,
        &output,
        deviceInfo->defaultSampleRate,
        m_numSamplesPerFrame,
        0,
        &Audio::callback,
        this);

#if !NX_DISABLE_AUDIO
    Pa_StartStream(m_stream);
#endif
}

Audio::~Audio()
{
    Pa_StopStream(m_stream);
    Pa_Terminate();
    delete[] m_soundBuffer;
}

void Audio::initialiseBuffers()
{
    // Each buffer needs to hold enough samples for a frame.  We'll double-buffer it so that one is the play buffer
    // and the other is the fill buffer.
    if (m_soundBuffer) delete[] m_soundBuffer;

    int numSamples = m_numSamplesPerFrame * 2;
    m_soundBuffer = new i16[numSamples];
    m_playBuffer = m_soundBuffer;
    m_fillBuffer = m_soundBuffer + m_numSamplesPerFrame;

    for (int i = 0; i < numSamples; ++i) m_soundBuffer[i] = 0;
}

int Audio::callback(const void *input,
    void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    Audio* self = (Audio *)userData;
    i16* outputBuffer = (i16 *)output;
    memcpy(outputBuffer, self->m_playBuffer, frameCount * sizeof(i16));
//    self->m_frameFunc();
    self->m_renderSignal.trigger();

    return paContinue;
}

void Audio::updateBeeper(i64 tState, u8 speaker)
{
    if (m_writePosition < m_numSamplesPerFrame)
    {
        i64 dt = tState - m_tStatesUpdated;
        if (m_tStateCounter + dt > m_numTStatesPerSample)
        {
            m_audioValue += int(speaker ? (m_numTStatesPerSample - m_tStateCounter) : 0);
            m_fillBuffer[m_writePosition++] = ((m_audioValue * 65535) / m_numTStatesPerSample) - 32768;

            dt = (m_tStateCounter + dt) - m_numTStatesPerSample;
            m_audioValue = 0;
            m_tStateCounter = 0;
        }

        m_audioValue += int(speaker ? dt : 0);
        m_tStateCounter += dt;
    }
    m_tStatesUpdated = tState;

    if (tState >= m_numTStatesPerFrame)
    {
        m_writePosition = 0;
        std::swap(m_fillBuffer, m_playBuffer);
        m_tStatesUpdated -= m_numTStatesPerFrame;
    }


    //     i64 beforeIndex = m_tStatesUpdated / m_numTStatesPerSample;
    //     i64 afterIndex = tState / m_numTStatesPerSample;
    //     if (afterIndex > m_numSamplesPerFrame) afterIndex = m_numSamplesPerFrame;
    // 
    //     for (i64 i = beforeIndex; i < afterIndex; ++i)
    //     {
    //         m_fillBuffer[i] = speaker ? 32767 : -32767;
    //     }
    // 
    //     m_tStatesUpdated = tState;
    // 
    //     // Check to see if we're done filling the buffer
    //     if (tState > m_numTStatesPerFrame)
    //     {
    //         std::swap(m_fillBuffer, m_playBuffer);
    //         m_tStatesUpdated = 0;
    //     }
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

