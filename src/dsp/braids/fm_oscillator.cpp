// Ported from Mutable Instruments Braids digital_oscillator.cc RenderFm
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#include "fm_oscillator.h"
#include "resources.h"
#include "../stmlib/dsp.h"

namespace braids {

static const uint16_t kHighestNote = 140 * 128;
static const uint16_t kPitchTableStart = 128 * 128;
static const uint16_t kOctave = 12 * 128;

void FmOscillator::Init()
{
    phase_ = 0;
    modulator_phase_ = 0;
    pitch_ = 0;
    parameter_[0] = 0;
    parameter_[1] = 0;
    previous_parameter_[0] = 0;
    previous_parameter_[1] = 0;
}

uint32_t FmOscillator::ComputePhaseIncrement(int16_t midi_pitch)
{
    if (midi_pitch >= kPitchTableStart) {
        midi_pitch = kPitchTableStart - 1;
    }

    int32_t ref_pitch = midi_pitch;
    ref_pitch -= kPitchTableStart;

    size_t num_shifts = 0;
    while (ref_pitch < 0) {
        ref_pitch += kOctave;
        ++num_shifts;
    }

    uint32_t a = lut_oscillator_increments[ref_pitch >> 4];
    uint32_t b = lut_oscillator_increments[(ref_pitch >> 4) + 1];
    uint32_t phase_increment = a +
        (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
    phase_increment >>= num_shifts;
    return phase_increment;
}

void FmOscillator::Render(int16_t* buffer, size_t size)
{
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);

    // Clamp pitch
    int16_t clamped_pitch = pitch_;
    if (clamped_pitch > kHighestNote) {
        clamped_pitch = kHighestNote;
    } else if (clamped_pitch < 0) {
        clamped_pitch = 0;
    }

    uint32_t modulator_phase = modulator_phase_;
    uint32_t modulator_phase_increment = ComputePhaseIncrement(
        (12 << 7) + clamped_pitch + ((parameter_[1] - 16384) >> 1)) >> 1;

    // Parameter interpolation
    int32_t parameter_0 = previous_parameter_[0];
    int32_t parameter_0_increment =
        (parameter_[0] - previous_parameter_[0]) / static_cast<int32_t>(size);

    while (size--) {
        parameter_0 += parameter_0_increment;

        phase_ += phase_increment;
        modulator_phase += modulator_phase_increment;

        uint32_t pm = (stmlib::Interpolate824(wav_sine, modulator_phase) *
                       parameter_0) << 2;
        *buffer++ = stmlib::Interpolate824(wav_sine, phase_ + pm);
    }

    previous_parameter_[0] = parameter_[0];
    modulator_phase_ = modulator_phase;
}

} // namespace braids
