#include <gtest/gtest.h>
#include "dsp/braids/analog_oscillator.h"

TEST(AnalogOscillator, InitDoesNotCrash)
{
    braids::AnalogOscillator osc;
    osc.Init();
}

TEST(AnalogOscillator, RenderSawProducesOutput)
{
    braids::AnalogOscillator osc;
    osc.Init();
    osc.set_shape(braids::OSC_SHAPE_SAW);
    osc.set_pitch(60 << 7);  // Middle C

    int16_t buffer[32];
    uint8_t sync[32] = {0};
    osc.Render(sync, buffer, 32);

    bool hasNonZero = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] != 0) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(AnalogOscillator, RenderSquareProducesOutput)
{
    braids::AnalogOscillator osc;
    osc.Init();
    osc.set_shape(braids::OSC_SHAPE_SQUARE);
    osc.set_pitch(60 << 7);

    int16_t buffer[32];
    uint8_t sync[32] = {0};
    osc.Render(sync, buffer, 32);

    // Square wave should have samples at extremes
    bool hasPositive = false;
    bool hasNegative = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] > 16000) hasPositive = true;
        if (buffer[i] < -16000) hasNegative = true;
    }
    EXPECT_TRUE(hasPositive || hasNegative);
}

TEST(AnalogOscillator, RenderTriangleProducesOutput)
{
    braids::AnalogOscillator osc;
    osc.Init();
    osc.set_shape(braids::OSC_SHAPE_TRIANGLE);
    osc.set_pitch(60 << 7);

    int16_t buffer[32];
    uint8_t sync[32] = {0};
    osc.Render(sync, buffer, 32);

    bool hasNonZero = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] != 0) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(AnalogOscillator, RenderSineProducesOutput)
{
    braids::AnalogOscillator osc;
    osc.Init();
    osc.set_shape(braids::OSC_SHAPE_SINE);
    osc.set_pitch(60 << 7);

    int16_t buffer[32];
    uint8_t sync[32] = {0};
    osc.Render(sync, buffer, 32);

    bool hasNonZero = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] != 0) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(AnalogOscillator, DifferentShapesProduceDifferentOutput)
{
    braids::AnalogOscillator osc;
    uint8_t sync[32] = {0};

    // Render saw
    osc.Init();
    osc.set_shape(braids::OSC_SHAPE_SAW);
    osc.set_pitch(60 << 7);
    int16_t saw_buffer[32];
    osc.Render(sync, saw_buffer, 32);

    // Render square
    osc.Init();
    osc.set_shape(braids::OSC_SHAPE_SQUARE);
    osc.set_pitch(60 << 7);
    int16_t square_buffer[32];
    osc.Render(sync, square_buffer, 32);

    // They should be different
    bool different = false;
    for (int i = 0; i < 32; ++i) {
        if (saw_buffer[i] != square_buffer[i]) different = true;
    }
    EXPECT_TRUE(different);
}
