// Ported from Mutable Instruments Braids
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include <cstddef>

namespace braids {

// Sine wavetable (257 entries for interpolation)
extern const int16_t wav_sine[];
extern const size_t WAV_SINE_SIZE;

// Oscillator phase increment lookup table
extern const uint32_t lut_oscillator_increments[];
extern const size_t LUT_OSCILLATOR_INCREMENTS_SIZE;

// FM frequency quantizer
extern const int16_t lut_fm_frequency_quantizer[];
extern const size_t LUT_FM_FREQUENCY_QUANTIZER_SIZE;

// Waveshaping tables (257 entries each)
extern const int16_t ws_sine_fold[];
extern const int16_t ws_tri_fold[];
extern const int16_t ws_violent_overdrive[];

// SVF filter cutoff coefficient table
extern const int16_t lut_svf_cutoff[];

} // namespace braids
