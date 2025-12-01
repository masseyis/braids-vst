#include <gtest/gtest.h>
#include "dsp/braids/macro_oscillator.h"

TEST(MacroOscillator, InitDoesNotCrash)
{
    braids::MacroOscillator osc;
    osc.Init();
}

TEST(MacroOscillator, SetShapeWorks)
{
    braids::MacroOscillator osc;
    osc.Init();
    osc.set_shape(braids::MACRO_OSC_SHAPE_FM);
    // Should not crash
}

TEST(MacroOscillator, RenderFmProducesOutput)
{
    braids::MacroOscillator osc;
    osc.Init();
    osc.set_shape(braids::MACRO_OSC_SHAPE_FM);
    osc.set_pitch(60 << 7);
    osc.set_parameters(8192, 16384);

    int16_t buffer[32];
    uint8_t sync[32] = {0};
    osc.Render(sync, buffer, 32);

    bool hasNonZero = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] != 0) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(MacroOscillator, ShapeChangesOutput)
{
    braids::MacroOscillator osc;
    osc.Init();
    osc.set_pitch(60 << 7);
    osc.set_parameters(8192, 16384);

    // Render with FM
    int16_t buffer_fm[32];
    uint8_t sync[32] = {0};
    osc.set_shape(braids::MACRO_OSC_SHAPE_FM);
    osc.Render(sync, buffer_fm, 32);

    // Reset and render with CSAW (will use analog oscillator)
    osc.Init();
    osc.set_pitch(60 << 7);
    osc.set_parameters(8192, 16384);
    osc.set_shape(braids::MACRO_OSC_SHAPE_CSAW);

    int16_t buffer_csaw[32];
    osc.Render(sync, buffer_csaw, 32);

    // They should produce different output
    bool different = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer_fm[i] != buffer_csaw[i]) different = true;
    }
    EXPECT_TRUE(different);
}
