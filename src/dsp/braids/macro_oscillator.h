// Ported from Mutable Instruments Braids macro_oscillator.h
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include "../stmlib/stmlib.h"
#include "analog_oscillator.h"
#include "fm_oscillator.h"

namespace braids {

enum MacroOscillatorShape {
    MACRO_OSC_SHAPE_CSAW,           // 0 - Classic saw
    MACRO_OSC_SHAPE_MORPH,          // 1 - Morphing waveforms
    MACRO_OSC_SHAPE_SAW_SQUARE,     // 2 - Saw to square
    MACRO_OSC_SHAPE_SINE_TRIANGLE,  // 3 - Sine to triangle
    MACRO_OSC_SHAPE_BUZZ,           // 4 - Buzz
    MACRO_OSC_SHAPE_SQUARE_SUB,     // 5 - Square with sub
    MACRO_OSC_SHAPE_SAW_SUB,        // 6 - Saw with sub
    MACRO_OSC_SHAPE_SQUARE_SYNC,    // 7 - Hard sync square
    MACRO_OSC_SHAPE_SAW_SYNC,       // 8 - Hard sync saw
    MACRO_OSC_SHAPE_FM,             // 9 - FM
    MACRO_OSC_SHAPE_LAST
};

class MacroOscillator {
public:
    MacroOscillator() = default;
    ~MacroOscillator() = default;

    void Init();

    void set_shape(MacroOscillatorShape shape) { shape_ = shape; }
    MacroOscillatorShape shape() const { return shape_; }

    void set_pitch(int16_t pitch) { pitch_ = pitch; }
    void set_parameters(int16_t p1, int16_t p2) {
        parameter_[0] = p1;
        parameter_[1] = p2;
    }

    void Render(const uint8_t* sync, int16_t* buffer, size_t size);

private:
    MacroOscillatorShape shape_ = MACRO_OSC_SHAPE_FM;
    int16_t pitch_ = 0;
    int16_t parameter_[2] = {0, 0};

    AnalogOscillator analog_oscillator_;
    FmOscillator fm_oscillator_;

    DISALLOW_COPY_AND_ASSIGN(MacroOscillator);
};

} // namespace braids
