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
    aux_parameter_ = 0;
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
        case OSC_SHAPE_VARIABLE_SAW:
            RenderVariableSaw(sync, buffer, size);
            break;
        case OSC_SHAPE_CSAW:
            RenderCSaw(sync, buffer, size);
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
        case OSC_SHAPE_TRIANGLE_FOLD:
            RenderTriangleFold(sync, buffer, size);
            break;
        case OSC_SHAPE_SINE_FOLD:
            RenderSineFold(sync, buffer, size);
            break;
        case OSC_SHAPE_BUZZ:
            RenderBuzz(sync, buffer, size);
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

void AnalogOscillator::RenderVariableSaw(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Variable sawtooth - parameter controls the transition point
    // At parameter=0: standard sawtooth
    // As parameter increases: transition point moves, creating different harmonic content
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    // Minimum pulse width to avoid clicks
    int32_t pw = 1024 + (parameter_ > 0 ? parameter_ : 0);
    if (pw > 32000) pw = 32000;
    uint32_t pw_32 = static_cast<uint32_t>(pw) << 16;

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Variable saw: ramp up to pw, then ramp down
        int32_t sample;
        if (phase < pw_32) {
            // Rising portion
            sample = static_cast<int32_t>((phase >> 15) * 32767 / (pw_32 >> 15)) - 32768;
        } else {
            // Falling portion
            uint32_t remaining = 0xFFFFFFFF - pw_32;
            uint32_t pos = phase - pw_32;
            sample = 32767 - static_cast<int32_t>((pos >> 15) * 65535 / (remaining >> 15));
        }

        *buffer++ = static_cast<int16_t>(stmlib::Clip16(sample));
    }
    phase_ = phase;
}

void AnalogOscillator::RenderCSaw(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // CSAW - Classic/Corrected Sawtooth with waveshaping
    // Parameter controls the amount of waveshaping/harmonics
    // Aux parameter controls DC offset/brightness
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    // Waveshaping amount from parameter
    int32_t shape_amount = parameter_;
    if (shape_amount < 0) shape_amount = 0;

    // DC offset from aux_parameter
    int16_t dc_shift = static_cast<int16_t>(-(aux_parameter_ - 32767) >> 4);

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Basic saw
        int32_t saw = static_cast<int32_t>((phase >> 16) - 32768);

        // Apply waveshaping based on parameter
        // This creates harmonics similar to the original Braids CSAW
        if (shape_amount > 0) {
            // Simple waveshaping: add some fold/clip character
            int32_t shaped = saw + ((saw * shape_amount) >> 16);
            saw = stmlib::Clip16(shaped);
        }

        // Apply DC offset and gain
        int32_t sample = saw + dc_shift;
        sample = (sample * 13) >> 3;  // ~1.6x gain like original

        *buffer++ = static_cast<int16_t>(stmlib::Clip16(sample));
    }
    phase_ = phase;
}

void AnalogOscillator::RenderTriangleFold(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Triangle with wavefolder - parameter controls fold amount
    // Creates rich harmonics when driven hard
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    // Gain from parameter: 2048 + (parameter * 30720 >> 15)
    // This matches the original Braids formula
    int32_t gain = 2048 + ((parameter_ * 30720) >> 15);

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Generate triangle
        int32_t tri = phase >> 15;  // 0 to 131071
        if (tri > 65535) {
            tri = 131071 - tri;  // Fold back
        }
        tri -= 32768;  // Center at 0: -32768 to 32767

        // Apply gain for folding
        int32_t amplified = (tri * gain) >> 11;

        // Wavefold using lookup table
        // Map amplified value to table index (0-256)
        int32_t index = (amplified + 32768) >> 8;
        if (index < 0) index = 0;
        if (index > 256) index = 256;

        *buffer++ = ws_tri_fold[index];
    }
    phase_ = phase;
}

void AnalogOscillator::RenderSineFold(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Sine with wavefolder - parameter controls fold amount
    // Smoother folding character than triangle
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    // Gain from parameter: 2048 + (parameter * 30720 >> 15)
    int32_t gain = 2048 + ((parameter_ * 30720) >> 15);

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Generate sine
        int16_t sine = stmlib::Interpolate824(wav_sine, phase);

        // Apply gain for folding
        int32_t amplified = (static_cast<int32_t>(sine) * gain) >> 11;

        // Wavefold using lookup table
        int32_t index = (amplified + 32768) >> 8;
        if (index < 0) index = 0;
        if (index > 256) index = 256;

        *buffer++ = ws_sine_fold[index];
    }
    phase_ = phase;
}

void AnalogOscillator::RenderBuzz(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Buzz - parameter controls harmonic density/brightness
    // Creates a buzzy, comb-like sound
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);
    uint32_t phase = phase_;

    // Parameter affects the "buzziness" - number of harmonics
    // Higher parameter = more harmonics = brighter buzz
    int32_t harmonics = 2 + (parameter_ >> 11);  // 2-18 harmonics
    if (harmonics > 16) harmonics = 16;

    while (size--) {
        if (*sync++) {
            phase = 0;
        }
        phase += phase_increment;

        // Generate buzz by summing harmonics with alternating phase
        // This creates a comb-filter like effect
        int32_t sum = 0;
        uint32_t harmonic_phase = phase;
        int32_t amplitude = 32767;

        for (int32_t h = 1; h <= harmonics; ++h) {
            int16_t harmonic = stmlib::Interpolate824(wav_sine, harmonic_phase);
            sum += (harmonic * amplitude) >> 15;
            harmonic_phase += phase_increment;  // Each harmonic is higher frequency
            amplitude = (amplitude * 28000) >> 15;  // Decay amplitude for higher harmonics
        }

        *buffer++ = static_cast<int16_t>(stmlib::Clip16(sum));
    }
    phase_ = phase;
}

} // namespace braids
