// Ported from Mutable Instruments Braids macro_oscillator.cc
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#include "macro_oscillator.h"
#include "resources.h"
#include "../stmlib/dsp.h"

namespace braids {

void MacroOscillator::Init()
{
    analog_oscillator_[0].Init();
    analog_oscillator_[1].Init();
    fm_oscillator_.Init();
    shape_ = MACRO_OSC_SHAPE_FM;
    pitch_ = 0;
    parameter_[0] = 0;
    parameter_[1] = 0;
    lp_state_ = 0;

    for (size_t i = 0; i < 128; ++i) {
        temp_buffer_[i] = 0;
    }
}

void MacroOscillator::Render(const uint8_t* sync, int16_t* buffer, size_t size)
{
    switch (shape_) {
        case MACRO_OSC_SHAPE_CSAW:
            RenderCSaw(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_MORPH:
            RenderMorph(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SAW_SQUARE:
            RenderSawSquare(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SINE_TRIANGLE:
            RenderSineTriangle(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_BUZZ:
            RenderBuzz(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SQUARE_SUB:
        case MACRO_OSC_SHAPE_SAW_SUB:
            RenderSub(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SQUARE_SYNC:
        case MACRO_OSC_SHAPE_SAW_SYNC:
            RenderSync(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_FM:
        default:
            fm_oscillator_.set_pitch(pitch_);
            fm_oscillator_.set_parameters(parameter_[0], parameter_[1]);
            fm_oscillator_.Render(buffer, size);
            break;
    }
}

void MacroOscillator::RenderCSaw(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // CSAW - Classic sawtooth with waveshaping controlled by timbre
    // Color controls brightness/DC offset
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_shape(OSC_SHAPE_CSAW);
    analog_oscillator_[0].set_parameter(parameter_[0]);
    analog_oscillator_[0].set_aux_parameter(parameter_[1]);
    analog_oscillator_[0].Render(sync, buffer, size);
}

void MacroOscillator::RenderMorph(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Morph - Crossfade between waveforms based on timbre
    // Timbre morphs: Triangle -> Saw -> Square -> PWM
    // Color adds lowpass filtering and overdrive

    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[1].set_pitch(pitch_);

    uint16_t balance;
    if (parameter_[0] <= 10922) {
        // Triangle to Saw morphing (0-33%)
        analog_oscillator_[0].set_shape(OSC_SHAPE_TRIANGLE);
        analog_oscillator_[1].set_shape(OSC_SHAPE_SAW);
        balance = static_cast<uint16_t>(parameter_[0] * 6);
    } else if (parameter_[0] <= 21845) {
        // Saw to Square morphing (33-66%)
        analog_oscillator_[0].set_shape(OSC_SHAPE_SAW);
        analog_oscillator_[1].set_shape(OSC_SHAPE_SQUARE);
        balance = static_cast<uint16_t>((parameter_[0] - 10923) * 6);
    } else {
        // Square with PWM (66-100%)
        analog_oscillator_[0].set_shape(OSC_SHAPE_SQUARE);
        analog_oscillator_[0].set_parameter((parameter_[0] - 21846) * 3);
        analog_oscillator_[1].set_shape(OSC_SHAPE_SQUARE);
        balance = 0;  // Just use first oscillator with PWM
    }

    // Render both oscillators
    analog_oscillator_[0].Render(sync, buffer, size);

    if (balance > 0) {
        analog_oscillator_[1].Render(sync, temp_buffer_, size);

        // Mix based on balance
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = stmlib::Mix(buffer[i], temp_buffer_[i], balance);
        }
    }

    // Apply filtering and overdrive based on color parameter
    int32_t lp_cutoff = pitch_ - (parameter_[1] >> 1) + 128 * 128;
    if (lp_cutoff < 0) lp_cutoff = 0;
    if (lp_cutoff > 32767) lp_cutoff = 32767;

    int32_t f = stmlib::Interpolate824(lut_svf_cutoff, static_cast<uint32_t>(lp_cutoff) << 17);
    int32_t fuzz_amount = parameter_[1] << 1;

    // Reduce fuzz at high pitches to avoid aliasing
    if (pitch_ > (80 << 7)) {
        fuzz_amount -= (pitch_ - (80 << 7)) << 4;
        if (fuzz_amount < 0) fuzz_amount = 0;
    }

    for (size_t i = 0; i < size; ++i) {
        int32_t sample = buffer[i];
        lp_state_ += (sample - lp_state_) * f >> 15;
        if (lp_state_ > 32767) lp_state_ = 32767;
        if (lp_state_ < -32768) lp_state_ = -32768;

        // Apply overdrive
        int32_t shifted = lp_state_ + 32768;
        int32_t index = shifted >> 8;
        if (index > 256) index = 256;
        int16_t fuzzed = ws_violent_overdrive[index];

        buffer[i] = stmlib::Mix(static_cast<int16_t>(lp_state_), fuzzed, static_cast<uint16_t>(fuzz_amount));
    }
}

void MacroOscillator::RenderSawSquare(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Saw/Square crossfade
    // Timbre: Variable saw shape
    // Color: Crossfade amount between saw and square

    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[1].set_pitch(pitch_);

    analog_oscillator_[0].set_shape(OSC_SHAPE_VARIABLE_SAW);
    analog_oscillator_[0].set_parameter(parameter_[0]);

    analog_oscillator_[1].set_shape(OSC_SHAPE_SQUARE);
    analog_oscillator_[1].set_parameter(parameter_[0]);  // PWM follows timbre

    // Render both
    analog_oscillator_[0].Render(sync, buffer, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, size);

    // Crossfade based on color
    uint16_t balance = static_cast<uint16_t>(parameter_[1] << 1);

    for (size_t i = 0; i < size; ++i) {
        // Attenuate square slightly for better mix
        int16_t attenuated_square = static_cast<int16_t>(
            (static_cast<int32_t>(temp_buffer_[i]) * 148) >> 8);
        buffer[i] = stmlib::Mix(buffer[i], attenuated_square, balance);
    }
}

void MacroOscillator::RenderSineTriangle(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Sine/Triangle with wavefold
    // Timbre: Fold amount (adds harmonics)
    // Color: Crossfade between sine fold and triangle fold

    // Pitch-dependent attenuation to prevent aliasing at high frequencies
    int32_t attenuation_sine = 32767 - 6 * (pitch_ - (92 << 7));
    int32_t attenuation_tri = 32767 - 7 * (pitch_ - (80 << 7));
    if (attenuation_tri < 0) attenuation_tri = 0;
    if (attenuation_sine < 0) attenuation_sine = 0;
    if (attenuation_tri > 32767) attenuation_tri = 32767;
    if (attenuation_sine > 32767) attenuation_sine = 32767;

    int32_t timbre = parameter_[0];

    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[1].set_pitch(pitch_);

    analog_oscillator_[0].set_shape(OSC_SHAPE_SINE_FOLD);
    analog_oscillator_[0].set_parameter(static_cast<int16_t>((timbre * attenuation_sine) >> 15));

    analog_oscillator_[1].set_shape(OSC_SHAPE_TRIANGLE_FOLD);
    analog_oscillator_[1].set_parameter(static_cast<int16_t>((timbre * attenuation_tri) >> 15));

    // Render both
    analog_oscillator_[0].Render(sync, buffer, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, size);

    // Crossfade based on color
    uint16_t balance = static_cast<uint16_t>(parameter_[1] << 1);

    for (size_t i = 0; i < size; ++i) {
        buffer[i] = stmlib::Mix(buffer[i], temp_buffer_[i], balance);
    }
}

void MacroOscillator::RenderBuzz(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Buzz - two detuned buzz oscillators
    // Timbre: Buzz character/harmonics
    // Color: Detune amount between the two oscillators

    analog_oscillator_[0].set_shape(OSC_SHAPE_BUZZ);
    analog_oscillator_[0].set_parameter(parameter_[0]);
    analog_oscillator_[0].set_pitch(pitch_);

    analog_oscillator_[1].set_shape(OSC_SHAPE_BUZZ);
    analog_oscillator_[1].set_parameter(parameter_[0]);
    // Detune second oscillator based on color
    int16_t detune = static_cast<int16_t>(parameter_[1] >> 8);
    analog_oscillator_[1].set_pitch(pitch_ + detune);

    // Render both
    analog_oscillator_[0].Render(sync, buffer, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, size);

    // Mix 50/50
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = (buffer[i] >> 1) + (temp_buffer_[i] >> 1);
    }
}

void MacroOscillator::RenderSub(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Sub oscillator - main osc + sub one octave below
    // Timbre: Main oscillator character (PWM for square, variable for saw)
    // Color: Sub oscillator mix amount

    bool is_square = (shape_ == MACRO_OSC_SHAPE_SQUARE_SUB);

    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_shape(is_square ? OSC_SHAPE_SQUARE : OSC_SHAPE_VARIABLE_SAW);
    analog_oscillator_[0].set_parameter(parameter_[0]);

    // Sub oscillator one octave below
    analog_oscillator_[1].set_pitch(pitch_ - (12 << 7));  // -12 semitones
    analog_oscillator_[1].set_shape(is_square ? OSC_SHAPE_SQUARE : OSC_SHAPE_SAW);

    // Render both
    analog_oscillator_[0].Render(sync, buffer, size);
    analog_oscillator_[1].Render(sync, temp_buffer_, size);

    // Mix based on color
    uint16_t sub_level = static_cast<uint16_t>(parameter_[1] << 1);

    for (size_t i = 0; i < size; ++i) {
        int32_t mixed = buffer[i] + ((temp_buffer_[i] * sub_level) >> 16);
        buffer[i] = static_cast<int16_t>(stmlib::Clip16(mixed));
    }
}

void MacroOscillator::RenderSync(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Hard sync oscillator
    // Timbre: Slave oscillator pitch ratio
    // Color: Waveshaping amount

    bool is_square = (shape_ == MACRO_OSC_SHAPE_SQUARE_SYNC);

    // Master oscillator at base pitch
    analog_oscillator_[0].set_pitch(pitch_);
    analog_oscillator_[0].set_shape(is_square ? OSC_SHAPE_SQUARE : OSC_SHAPE_SAW);

    // Slave oscillator pitch controlled by timbre
    // Higher timbre = higher slave pitch = more harmonics
    int16_t slave_pitch = pitch_ + (parameter_[0] >> 2);
    analog_oscillator_[1].set_pitch(slave_pitch);
    analog_oscillator_[1].set_shape(is_square ? OSC_SHAPE_SQUARE : OSC_SHAPE_SAW);

    // Generate sync signal from master
    uint8_t sync_buffer[128];
    for (size_t i = 0; i < size; ++i) {
        sync_buffer[i] = 0;
    }

    // Render master to generate sync points
    analog_oscillator_[0].Render(sync, temp_buffer_, size);

    // Simple sync detection: look for zero crossings in master
    for (size_t i = 1; i < size; ++i) {
        if (temp_buffer_[i-1] < 0 && temp_buffer_[i] >= 0) {
            sync_buffer[i] = 1;
        }
    }

    // Render slave with sync
    analog_oscillator_[1].Render(sync_buffer, buffer, size);

    // Apply waveshaping based on color
    if (parameter_[1] > 0) {
        int32_t shape_amount = parameter_[1];
        for (size_t i = 0; i < size; ++i) {
            int32_t sample = buffer[i];
            // Simple waveshaping
            sample = sample + ((sample * shape_amount) >> 16);
            buffer[i] = static_cast<int16_t>(stmlib::Clip16(sample));
        }
    }
}

} // namespace braids
