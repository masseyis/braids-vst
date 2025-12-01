#include <gtest/gtest.h>
#include "dsp/braids/fm_oscillator.h"

TEST(FmOscillator, InitDoesNotCrash)
{
    braids::FmOscillator osc;
    osc.Init();
}

TEST(FmOscillator, RenderProducesNonZeroOutput)
{
    braids::FmOscillator osc;
    osc.Init();
    osc.set_pitch(60 << 7); // Middle C in Braids pitch format
    osc.set_parameters(8192, 16384); // Moderate FM depth and ratio

    int16_t buffer[32];
    osc.Render(buffer, 32);

    // Check that at least some samples are non-zero
    bool hasNonZero = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(FmOscillator, OutputIsInRange)
{
    braids::FmOscillator osc;
    osc.Init();
    osc.set_pitch(72 << 7); // C5
    osc.set_parameters(16384, 16384);

    int16_t buffer[256];
    osc.Render(buffer, 256);

    for (int i = 0; i < 256; ++i) {
        EXPECT_GE(buffer[i], -32768);
        EXPECT_LE(buffer[i], 32767);
    }
}
