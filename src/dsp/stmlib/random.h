// Ported from Mutable Instruments stmlib/utils/random.h
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>

namespace stmlib {

class Random
{
public:
    static void Seed(uint32_t seed)
    {
        state_ = seed;
    }

    static uint32_t GetWord()
    {
        state_ = state_ * 1664525L + 1013904223L;
        return state_;
    }

    static int16_t GetSample()
    {
        return static_cast<int16_t>(GetWord() >> 16);
    }

    static float GetFloat()
    {
        return static_cast<float>(GetWord()) / 4294967296.0f;
    }

private:
    static inline uint32_t state_ = 0x12345678;
};

} // namespace stmlib
