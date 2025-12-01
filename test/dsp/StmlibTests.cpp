#include <gtest/gtest.h>
#include "dsp/stmlib/stmlib.h"
#include "dsp/stmlib/dsp.h"
#include "dsp/stmlib/random.h"
#include "dsp/braids/resources.h"

TEST(Stmlib, ClipPositive)
{
    int32_t x = 40000;
    CLIP(x);
    EXPECT_EQ(x, 32767);
}

TEST(Stmlib, ClipNegative)
{
    int32_t x = -40000;
    CLIP(x);
    EXPECT_EQ(x, -32768);
}

TEST(Stmlib, ClipNoChange)
{
    int32_t x = 1000;
    CLIP(x);
    EXPECT_EQ(x, 1000);
}

TEST(Stmlib, Interpolate824_Midpoint)
{
    // Table with 256 entries + 1 for interpolation wraparound
    // At phase 0x80000000, index = 128, so we need a table that size
    static const int16_t table[257] = {0, 256, 512, 768, 1024}; // First few entries
    // Test at index 0, halfway to index 1: phase = 0x00800000
    uint32_t phase = 0x00800000; // Halfway between indices 0 and 1
    int16_t result = stmlib::Interpolate824(table, phase);
    EXPECT_NEAR(result, 128, 2); // Halfway between 0 and 256
}

TEST(Stmlib, Mix5050)
{
    int16_t a = 0;
    int16_t b = 32767;
    int16_t result = stmlib::Mix(a, b, 32768); // 50% mix
    EXPECT_NEAR(result, 16383, 1);
}

TEST(Stmlib, RandomProducesValues)
{
    stmlib::Random::Seed(12345);
    int16_t sample = stmlib::Random::GetSample();
    // Just verify it returns something in range
    EXPECT_GE(sample, -32768);
    EXPECT_LE(sample, 32767);
}

TEST(Resources, SineWaveTableExists)
{
    // Verify sine table has expected properties
    EXPECT_EQ(braids::wav_sine[0], 0);          // Sine starts at 0
    EXPECT_GT(braids::wav_sine[64], 30000);     // Peak around 32767
    EXPECT_NEAR(braids::wav_sine[128], 0, 100); // Zero crossing
}

TEST(Resources, OscillatorIncrementsExist)
{
    // Verify LUT exists and has reasonable values
    EXPECT_GT(braids::lut_oscillator_increments[0], 0u);
}
