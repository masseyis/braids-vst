// Ported from Mutable Instruments stmlib/utils/dsp.h
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>

namespace stmlib {

// Linear interpolation with 8.24 fixed-point phase
inline int16_t Interpolate824(const int16_t* table, uint32_t phase)
{
    uint32_t index = phase >> 24;
    uint32_t frac = (phase >> 8) & 0xFFFF;
    int32_t a = table[index];
    int32_t b = table[index + 1];
    return static_cast<int16_t>(a + ((b - a) * static_cast<int32_t>(frac) >> 16));
}

// Linear interpolation for 8-bit tables with 8.24 phase
inline int16_t Interpolate824(const uint8_t* table, uint32_t phase)
{
    uint32_t index = phase >> 24;
    uint32_t frac = (phase >> 8) & 0xFFFF;
    int32_t a = static_cast<int16_t>(table[index]) - 128;
    int32_t b = static_cast<int16_t>(table[index + 1]) - 128;
    return static_cast<int16_t>((a + ((b - a) * static_cast<int32_t>(frac) >> 16)) << 8);
}

// Linear interpolation with 8.8 fixed-point phase
inline int16_t Interpolate88(const int16_t* table, uint16_t index)
{
    uint16_t integral = index >> 8;
    uint16_t frac = index & 0xFF;
    int32_t a = table[integral];
    int32_t b = table[integral + 1];
    return static_cast<int16_t>(a + ((b - a) * frac >> 8));
}

// 10.22 interpolation for delay lines
inline int16_t Interpolate1022(const int16_t* table, uint32_t phase)
{
    uint32_t index = phase >> 22;
    uint32_t frac = (phase >> 6) & 0xFFFF;
    int32_t a = table[index];
    int32_t b = table[index + 1];
    return static_cast<int16_t>(a + ((b - a) * static_cast<int32_t>(frac) >> 16));
}

// Mix two signals: result = a + (b - a) * balance / 65536
inline int16_t Mix(int16_t a, int16_t b, uint16_t balance)
{
    return static_cast<int16_t>(a + (((static_cast<int32_t>(b) - a) * balance) >> 16));
}

// Clip a 32-bit value to 16-bit signed range
inline int32_t Clip16(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return x;
}

// Crossfade between two wavetable entries
inline int16_t Crossfade(const uint8_t* table_a, const uint8_t* table_b,
                          uint32_t phase, uint16_t balance)
{
    int16_t a = Interpolate824(table_a, phase);
    int16_t b = Interpolate824(table_b, phase);
    return Mix(a, b, balance);
}

} // namespace stmlib
