// Ported from Mutable Instruments Braids digital_oscillator.cc RenderFm
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include <cstring>
#include "../stmlib/stmlib.h"

namespace braids {

class FmOscillator
{
public:
    FmOscillator() = default;
    ~FmOscillator() = default;

    void Init();

    void set_pitch(int16_t pitch) { pitch_ = pitch; }
    void set_parameters(int16_t param1, int16_t param2) {
        parameter_[0] = param1;
        parameter_[1] = param2;
    }

    void Render(int16_t* buffer, size_t size);

private:
    uint32_t ComputePhaseIncrement(int16_t midi_pitch);

    uint32_t phase_ = 0;
    uint32_t modulator_phase_ = 0;
    int16_t pitch_ = 0;
    int16_t parameter_[2] = {0, 0};
    int16_t previous_parameter_[2] = {0, 0};

    DISALLOW_COPY_AND_ASSIGN(FmOscillator);
};

} // namespace braids
