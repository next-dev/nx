//----------------------------------------------------------------------------------------------------------------------
// AY-3-8912 emulation
//----------------------------------------------------------------------------------------------------------------------

#include <audio/ay.h>

static const int kAYFrequency = 44100;
static const int kAYChannels = 2;
static const int kAYBits = 16;

static const int kAYMaxAmp = 24575;
static const int kAYChipFreq = 1773400;
static const int kAYTicksPerSample = (kAYChipFreq / kAYFrequency / 8);

//----------------------------------------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------------------------------------

AYChip::AYChip()
{
    reset(Type::AY);

    // Generate envelope tables                     CALH
    //
    //  0   \_______    Single decay then off       0000
    //
    //  4   /|______    Single attack then off      0100
    //
    //  8   \|\|\|\|    Repeated decay              1000
    //
    //  9   \_______    Single decay then off       1001
    //
    //  10  \/\/\/\/    Repeated decay-attack       1010
    //        ______
    //  11  \|          Single decay then hold      1011
    //
    //  12  /|/|/|/|    Repeated attack             1100
    //       _______
    //  13  /           Single attack then hold     1101
    //
    //  14  /\/\/\/\    Repeated attack-delay       1110
    //
    //  15  /|______    Single attack then off      1111
    //
    //
    //  C = Continue
    //  A = Initial attack
    //  L = aLternate
    //  H = Hold
    //

    for (int i = 0; i < 16; ++i)
    {
        bool hold = false;

        // Calculate initial pattern and volume
        int attack = (i & 4) ? 1 : -1;
        int volume = (i & 4) ? -1 : 32;

        for (int x = 0; x < 128; ++x)
        {
            if (!hold)
            {
                volume += attack;
                if (volume < 0 || volume >= 32)
                {
                    // Completed initial sound, calculate next pattern
                    if (i & 8)
                    {
                        // Sound continues.  If bit 2, alternate sound
                        if (i & 2) attack = -attack;
                        if (i & 1)
                        {
                            hold = true;
                            volume = (attack > 0) ? 31 : 0;
                        }
                        else
                        {
                            volume = (attack > 0) ? 0 : 31;
                        }
                    }
                    else
                    {
                        volume = 0;
                        hold = true;
                    }
                }
            }

            m_envelopes[i][x];
        }
    }
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

int gEqualiser[(int)AYChip::StereoMode::COUNT][6] =
{
    //  A-L     A-R     B-L     B-R     C-L     C-R
    {   2,      2,      2,      2,      2,      2       },      // Mono
    {   2,      0,      1,      1,      0,      2       },      // ABC
    {   2,      0,      0,      2,      1,      1,      },      // ACB
    {   1,      1,      2,      0,      0,      2       },      // BAC
    {   0,      2,      2,      0,      1,      1,      },      // BCA
    {   1,      1,      0,      2,      2,      0,      },      // CAB
    {   0,      2,      1,      1,      2,      0,      },      // CBA
};

void AYChip::reset(Type type, StereoMode stereoMode)
{
    m_stereoMode = stereoMode;
    m_envX = 0;
    m_dirty = true;

    // Set up the volume tables
    if (type == Type::AY)
    {
        for (int i = 0; i < 32; ++i)
        {
            m_table[i] = gAyTable[i / 2];
        }
    }
    else
    {
        for (int i = 0; i < 32; ++i)
        {
            m_table[i] = gYmTable[i];
        }
    }

    // Set up the equaliser tables
    int eq[3] = { 33, 70, 100 };
    if (type == Type::YM) eq[0] = 5;
    for (int i = 0; i < 6; ++i)
    {
        m_equaliser[i] = gEqualiser[(int)stereoMode][i];
    }

    for (int i = 0; i < 5; ++i) m_counters[i] = 0;

    //
    // Generate volumes that maps channel/volume to actual volume
    //
    for (int i = 0; i < 32; ++i)
    {
        int vol = m_table[i];       // Real volume level
        for (int c = 0; c < 6; ++c)
        {
            m_volumes[c][i] = (int)(((float)vol * m_equaliser[c]) / 100.0f);
        }
    }

    m_counters.fill(0);
    m_bits.fill(false);
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

    m_dirty = true;
}


void AYChip::setReg(Register reg, u8 x)
{
    switch (reg)
    {
    case Register::PitchA_Fine:
        m_regs.m_tone[0] = (m_regs.m_tone[0] & 0xf00) | x;
        break;

    case Register::PitchA_Coarse:
        m_regs.m_tone[0] = (m_regs.m_tone[0] & 0xff) | ((x & 0x0f) << 8);
        break;

    case Register::PitchB_Fine:
        m_regs.m_tone[1] = (m_regs.m_tone[1] & 0xf00) | x;
        break;

    case Register::PitchB_Coarse:
        m_regs.m_tone[1] = (m_regs.m_tone[1] & 0xff) | ((x & 0x0f) << 8);
        break;

    case Register::PitchC_Fine:
        m_regs.m_tone[2] = (m_regs.m_tone[2] & 0xf00) | x;
        break;

    case Register::PitchC_Coarse:
        m_regs.m_tone[2] = (m_regs.m_tone[2] & 0xff) | ((x & 0x0f) << 8);
        break;

    case Register::PitchNoise:
        m_regs.m_noise = x & 0x1f;
        break;

    case Register::Mixer:
        m_regs.m_mixerTone[0] = !(x & 0x01);
        m_regs.m_mixerTone[1] = !(x & 0x02);
        m_regs.m_mixerTone[2] = !(x & 0x04);
        m_regs.m_mixerNoise[0] = !(x & 0x08);
        m_regs.m_mixerNoise[1] = !(x & 0x10);
        m_regs.m_mixerNoise[2] = !(x & 0x20);
        break;

    case Register::VolumeA:
        m_regs.m_volume[0] = (x & 0x0f);
        m_regs.m_envelope[0] = (x & 0x10);
        break;

    case Register::VolumeB:
        m_regs.m_volume[0] = (x & 0x0f);
        m_regs.m_envelope[0] = (x & 0x10);
        break;

    case Register::VolumeC:
        m_regs.m_volume[0] = (x & 0x0f);
        m_regs.m_envelope[0] = (x & 0x10);
        break;

    case Register::EnvelopeDuration_Fine:
        m_regs.m_envFreq = (m_regs.m_envFreq & 0xff00) | x;
        break;

    case Register::EnvelopeDuration_Coarse:
        m_regs.m_envFreq = (m_regs.m_envFreq & 0x00ff) | (x << 8);
        break;

    case Register::EnvelopeShape:
        m_regs.m_envType = x & 0x0f;
        m_envX = 0;
        m_counters[CounterEnvelope] = 0;
        break;
    }

    m_dirty = true;
}

//----------------------------------------------------------------------------------------------------------------------
// Generate sound wave for playing
//----------------------------------------------------------------------------------------------------------------------

void AYChip::play(void* outBuf, size_t numFrames)
{
    if (m_dirty)
    {
        //
        // Calculate the maximum volume for each channel
        //
        int maxLeft = m_volumes[0][31] + m_volumes[2][31] + m_volumes[4][31];
        int maxRight = m_volumes[1][31] + m_volumes[3][31] + m_volumes[5][31];
        int volume = std::max(maxLeft, maxRight);
        m_amp = kAYTicksPerSample * volume / kAYMaxAmp;

        m_dirty = false;
    }

    u16* p16 = (u16 *)outBuf;

    while (numFrames-- > 0)
    {
        int left = 0;
        int right = 0;

        for (int t = 0; t < kAYTicksPerSample; ++t)
        {
            // Use counters to determine frequency of square waves for each channel
            for (int c = 0; c < 2; ++c)
            {
                if (++m_counters[c] >= m_regs.m_tone[c])
                {
                    m_counters[c] = 0;
                    m_bits[c] = !m_bits[c];
                }
            }

            // Generate Noise

            // Manage the envelopes
            if (++m_counters[CounterEnvelope] >= m_regs.m_envFreq)
            {
                m_counters[CounterEnvelope] = 0;
                if (++m_envX > 127)
                {
                    m_envX = 64;
                }
            }

            // Mix the envelopes in with the tones
            for (int c = 0; c < 2; ++c)
            {
                if ((m_bits[c] || !m_regs.m_mixerTone[c]) && (m_bits[3] || !m_regs.m_mixerNoise[c]))
                {
                    int vol = (m_regs.m_envelope[c])
                        ? m_envelopes[m_regs.m_envType][m_envX]
                        : m_regs.m_volume[c] * 2 + 1;
                    assert(vol >= 0 && vol < 32);
                    left += m_volumes[c * 2][vol];
                    right += m_volumes[c * 2 + 1][vol];
                }
            }
        } // for t

        left /= m_amp;
        right /= m_amp;

        // Write out the sound waves
        *p16++ = left;
        *p16++ = right;
    }
}

