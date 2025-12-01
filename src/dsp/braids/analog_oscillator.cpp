// Ported from Mutable Instruments Braids analog_oscillator.cc
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#include "analog_oscillator.h"
#include "resources.h"
#include "../stmlib/dsp.h"

namespace braids {

static const uint16_t kHighestNote = 140 * 128;
static const uint16_t kPitchTableStart = 128 * 128;
static const uint16_t kOctave = 12 * 128;

void AnalogOscillator::Init()
{
    phase_ = 0;
    next_sample_ = 0;
    high_ = false;
    shape_ = OSC_SHAPE_SAW;
    pitch_ = 0;
    parameter_ = 0;
}

uint32_t AnalogOscillator::ComputePhaseIncrement(int16_t midi_pitch)
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

void AnalogOscillator::Render(const uint8_t* sync, int16_t* buffer, size_t size)
{
    switch (shape_) {
        case OSC_SHAPE_SAW:
            RenderSaw(sync, buffer, size);
            break;
        case OSC_SHAPE_SQUARE:
            RenderSquare(sync, buffer, size);
            break;
        case OSC_SHAPE_TRIANGLE:
            RenderTriangle(sync, buffer, size);
            break;
        case OSC_SHAPE_SINE:
            RenderSine(sync, buffer, size);
            break;
        default:
            RenderSaw(sync, buffer, size);
            break;
    }
}

void AnalogOscillator::RenderSaw(const uint8_t* sync, int16_t* buffer, size_t size)
{
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Convert phase to saw wave: phase / 2^32 * 65536 - 32768
        // Simplified: top 16 bits minus 32768
        int16_t sample = static_cast<int16_t>((phase >> 16) - 32768);
        *buffer++ = sample;
    }
    phase_ = phase;
}

void AnalogOscillator::RenderSquare(const uint8_t* sync, int16_t* buffer, size_t size)
{
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    // PWM: parameter controls pulse width (0 = 50%, 32767 = ~100%)
    uint32_t pw = 0x80000000;
    if (parameter_ > 0) {
        pw += static_cast<uint32_t>(parameter_) << 16;
    }

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        int16_t sample = (phase < pw) ? 32767 : -32768;
        *buffer++ = sample;
    }
    phase_ = phase;
}

void AnalogOscillator::RenderTriangle(const uint8_t* sync, int16_t* buffer, size_t size)
{
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Triangle: fold the phase
        int32_t tri = phase >> 15;  // 0 to 131071
        if (tri > 65535) {
            tri = 131071 - tri;  // Fold back
        }
        tri -= 32768;  // Center at 0
        *buffer++ = static_cast<int16_t>(tri);
    }
    phase_ = phase;
}

void AnalogOscillator::RenderSine(const uint8_t* sync, int16_t* buffer, size_t size)
{
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Use sine wavetable with interpolation
        *buffer++ = stmlib::Interpolate824(wav_sine, phase);
    }
    phase_ = phase;
}

} // namespace braids
