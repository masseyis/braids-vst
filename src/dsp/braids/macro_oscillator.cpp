// Ported from Mutable Instruments Braids macro_oscillator.cc
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#include "macro_oscillator.h"
#include "resources.h"

namespace braids {

void MacroOscillator::Init()
{
    analog_oscillator_.Init();
    fm_oscillator_.Init();
    shape_ = MACRO_OSC_SHAPE_FM;
    pitch_ = 0;
    parameter_[0] = 0;
    parameter_[1] = 0;
}

void MacroOscillator::Render(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Configure sub-oscillators
    analog_oscillator_.set_pitch(pitch_);
    analog_oscillator_.set_parameter(parameter_[0]);
    fm_oscillator_.set_pitch(pitch_);
    fm_oscillator_.set_parameters(parameter_[0], parameter_[1]);

    switch (shape_) {
        case MACRO_OSC_SHAPE_CSAW:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SAW_SQUARE:
        case MACRO_OSC_SHAPE_MORPH:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SINE_TRIANGLE:
            analog_oscillator_.set_shape(OSC_SHAPE_TRIANGLE);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SQUARE_SUB:
        case MACRO_OSC_SHAPE_BUZZ:
            analog_oscillator_.set_shape(OSC_SHAPE_SQUARE);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SAW_SUB:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SQUARE_SYNC:
            analog_oscillator_.set_shape(OSC_SHAPE_SQUARE);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SAW_SYNC:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_FM:
        default:
            fm_oscillator_.Render(buffer, size);
            break;
    }
}

} // namespace braids
