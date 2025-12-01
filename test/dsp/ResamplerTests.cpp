#include <gtest/gtest.h>
#include "dsp/resampler.h"
#include <cmath>

TEST(Resampler, InitDoesNotCrash)
{
    Resampler resampler;
    resampler.Init(96000.0, 48000.0);
}

TEST(Resampler, RatioCalculation)
{
    Resampler resampler;
    resampler.Init(96000.0, 48000.0);
    EXPECT_DOUBLE_EQ(resampler.ratio(), 2.0);

    resampler.Init(96000.0, 44100.0);
    EXPECT_NEAR(resampler.ratio(), 96000.0 / 44100.0, 0.0001);
}

TEST(Resampler, Downsample2x)
{
    Resampler resampler;
    resampler.Init(96000.0, 48000.0);  // 2:1 ratio

    // Input at 96kHz: simple ramp
    int16_t input[64];
    for (int i = 0; i < 64; ++i) {
        input[i] = static_cast<int16_t>(i * 512);  // 0 to 32256
    }

    float output[32];
    size_t outputWritten = resampler.Process(input, 64, output, 32);

    // Should produce ~32 output samples for 64 input samples at 2:1 ratio
    EXPECT_GE(outputWritten, 30u);
    EXPECT_LE(outputWritten, 32u);

    // Output should be roughly half the input values (every other sample)
    EXPECT_NEAR(output[0], 0.0f, 0.1f);
}

TEST(Resampler, PreservesSignalContent)
{
    Resampler resampler;
    resampler.Init(96000.0, 48000.0);

    // Generate a sine wave at 96kHz
    int16_t input[192];
    for (int i = 0; i < 192; ++i) {
        input[i] = static_cast<int16_t>(16384.0 * sin(2.0 * M_PI * i / 48.0));  // ~2kHz at 96kHz
    }

    float output[96];
    size_t outputWritten = resampler.Process(input, 192, output, 96);

    // Should have non-zero output
    bool hasNonZero = false;
    float maxVal = 0.0f;
    for (size_t i = 0; i < outputWritten; ++i) {
        if (std::abs(output[i]) > 0.01f) hasNonZero = true;
        if (std::abs(output[i]) > maxVal) maxVal = std::abs(output[i]);
    }
    EXPECT_TRUE(hasNonZero);
    EXPECT_GT(maxVal, 0.3f);  // Should preserve amplitude reasonably
}

TEST(Resampler, HandlesNon2xRatio)
{
    Resampler resampler;
    resampler.Init(96000.0, 44100.0);  // ~2.177:1 ratio

    int16_t input[96];
    for (int i = 0; i < 96; ++i) {
        input[i] = static_cast<int16_t>(i * 341);
    }

    float output[48];
    size_t outputWritten = resampler.Process(input, 96, output, 48);

    // Should produce roughly 96 / 2.177 ≈ 44 samples
    EXPECT_GE(outputWritten, 40u);
    EXPECT_LE(outputWritten, 48u);
}
