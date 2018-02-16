//----------------------------------------------------------------------------------------------------------------------
// AY-3-8912 emulation
//----------------------------------------------------------------------------------------------------------------------

#include <audio/ay.h>

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

AYChip::AYChip()
{
    reset(Type::AY, 44100, 1, 16);
}

//----------------------------------------------------------------------------------------------------------------------
// reset
//----------------------------------------------------------------------------------------------------------------------

int gAyTable[16] =
{
    0,      513,    828,    1239,
    1923,   3238,   4926,   9110,
    10344,  17876,  24682,  30442,
    38844,  47270,  56402,  65535
};

int gYmTable[32] =
{
    0,      0,      190,    286,
    375,    470,    560,    664,
    866,    1130,   1515,   1803,
    2253,   2848,   3351,   3862,
    4844,   6058,   7290,   8559,
    10474,  12878,  15297,  17787,
    21500,  26172,  30866,  35676,
    42664,  50986,  58842,  65535
};

void AYChip::reset(Type type, int freq, int chans, int bits)
{
    switch (type)
    {
    case Type::AY:      m_table = gAyTable;     break;
    case Type::YM:      m_table = gYmTable;     break;
    }

    m_freq = freq;
    m_numChannels = chans;
    m_sampleSize = bits;

    assert(bits == 8 || bits == 16);
    assert(chans == 1 || chans == 2);
    assert(freq >= 50);
}

//----------------------------------------------------------------------------------------------------------------------
// Register setting
//----------------------------------------------------------------------------------------------------------------------

void AYChip::setRegs(vector<u8> regs)
{
    assert(regs[1] < 16);
    assert(regs[3] < 16);
    assert(regs[5] < 16);
    assert(regs[6] < 32);
    assert(regs[8] < 32);
    assert(regs[9] < 32);
    assert(regs[10] < 32);
    assert(regs[13] < 16);

    m_regs.m_tone[0] = int(regs[0]) + (int(regs[1] & 0x0f) << 8);
    m_regs.m_tone[1] = int(regs[2]) + (int(regs[3] & 0x0f) << 8);
    m_regs.m_tone[2] = int(regs[4]) + (int(regs[5] & 0x0f) << 8);
    m_regs.m_volume[0] = int(regs[8] & 0x0f);
    m_regs.m_volume[1] = int(regs[9] & 0x0f);
    m_regs.m_volume[2] = int(regs[10] & 0x0f);
    m_regs.m_envelope[0] = (regs[8] & 0x10) != 0;
    m_regs.m_envelope[1] = (regs[9] & 0x10) != 0;
    m_regs.m_envelope[2] = (regs[10] & 0x10) != 0;
    m_regs.m_mixerTone[0] = !(regs[7] & 0x01);
    m_regs.m_mixerTone[1] = !(regs[7] & 0x02);
    m_regs.m_mixerTone[2] = !(regs[7] & 0x04);
    m_regs.m_mixerNoise[0] = !(regs[7] & 0x08);
    m_regs.m_mixerNoise[1] = !(regs[7] & 0x10);
    m_regs.m_mixerNoise[2] = !(regs[7] & 0x20);
    m_regs.m_noise = regs[6] & 0x1f;
    m_regs.m_envFreq = int(regs[11]) + (int(regs[12] & 0x0f) << 8);
    if (regs[13] != 0xff)
    {
        m_regs.m_envType = regs[13] & 0x0f;
        m_counters[CounterEnvelope] = 0;
        m_envX = 0;
    }
}
