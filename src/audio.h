//----------------------------------------------------------------------------------------------------------------------
// Audio System
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "SFML/Audio.hpp"

#define NX_AUDIO_SAMPLERATE 44100

//----------------------------------------------------------------------------------------------------------------------
// Beeper audio stream
//----------------------------------------------------------------------------------------------------------------------

class AudioStream final : public sf::SoundStream
{
public:
    AudioStream(int numTStatesPerFrame);
    ~AudioStream();

    int numTStatesPerSample() const { return m_numTStatesPerFrame / numSamplesPerFrame(); }
    int numSamplesPerFrame() const { return NX_AUDIO_SAMPLERATE / 50; }

private:
    void initialiseBuffers();

    bool onGetData(Chunk& data) override;
    void onSeek(sf::Time timeOffset) override;

private:
    int             m_numTStatesPerFrame;
    i16*            m_soundBuffer;
    i16*            m_playBuffer;
    i16*            m_fillBuffer;
};

//----------------------------------------------------------------------------------------------------------------------
// Audio system
//----------------------------------------------------------------------------------------------------------------------

class Audio
{
public:
    Audio(int numTStatesPerFrame);

private:
    AudioStream   m_stream;
};

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPL

//----------------------------------------------------------------------------------------------------------------------
// BeeperAudioStream
//----------------------------------------------------------------------------------------------------------------------

AudioStream::AudioStream(int numTStatesPerFrame)
    : m_numTStatesPerFrame(numTStatesPerFrame)
    , m_soundBuffer(nullptr)
    , m_playBuffer(nullptr)
    , m_fillBuffer(nullptr)
{
    initialize(1, NX_AUDIO_SAMPLERATE);
    initialiseBuffers();
}

AudioStream::~AudioStream()
{
    delete[] m_soundBuffer;
}

void AudioStream::initialiseBuffers()
{
    // Each buffer needs to hold enough samples for a frame.  We'll double-buffer it so that one is the play buffer
    // and the other is the fill buffer.
    if (m_soundBuffer) delete[] m_soundBuffer;

    int numSamples = numSamplesPerFrame() * 2;
    m_soundBuffer = new i16[numSamples];
    m_playBuffer = m_soundBuffer;
    m_fillBuffer = m_soundBuffer + numSamplesPerFrame();

    int bufferSizeInSamples = numSamplesPerFrame();
    for (int i = 0; i < bufferSizeInSamples; ++i) m_playBuffer[i] = 32767;
    for (int i = 0; i < bufferSizeInSamples; ++i) m_fillBuffer[i] = -32767;
}

bool AudioStream::onGetData(Chunk& data)
{
    data.samples = m_fillBuffer;
    data.sampleCount = numSamplesPerFrame();
    std::swap(m_fillBuffer, m_playBuffer);

    return true;
}

void AudioStream::onSeek(sf::Time timeOffset)
{

}

//----------------------------------------------------------------------------------------------------------------------
// Audio
//----------------------------------------------------------------------------------------------------------------------

Audio::Audio(int numTStatesPerFrame)
    : m_stream(numTStatesPerFrame)
{
    m_stream.play();
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPL
