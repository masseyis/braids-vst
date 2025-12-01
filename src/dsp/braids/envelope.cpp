// Ported from Mutable Instruments Braids envelope.cc
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#include "envelope.h"

namespace braids {

void Envelope::Init()
{
    segment_ = ENV_SEGMENT_DEAD;
    phase_ = 0;
    phase_increment_ = 0;
    attack_ = 0;
    decay_ = 0;
    value_ = 0;
}

uint32_t Envelope::ComputeIncrement(uint16_t time_ms)
{
    // time_ms is the envelope time in milliseconds (passed directly from processor)
    // Attack: 0-500ms, Decay: 10-2000ms
    //
    // Phase accumulator is 32-bit, wraps at 2^32
    // At 96kHz internal sample rate:
    //   samples_needed = time_ms * 96
    //   increment = 2^32 / samples_needed
    //
    // Higher time_ms = more samples needed = lower increment = slower envelope

    // Minimum 1ms to avoid division by zero and ensure snappy response
    float time = static_cast<float>(time_ms);
    if (time < 1.0f) {
        time = 1.0f;
    }

    // Calculate samples needed at 96kHz
    float samples = time * 96.0f;

    // Calculate increment: 2^32 / samples
    uint32_t increment = static_cast<uint32_t>(4294967296.0f / samples);

    // Ensure minimum increment to prevent stuck envelopes
    if (increment < 100) {
        increment = 100;
    }

    return increment;
}

void Envelope::Trigger(uint16_t attack, uint16_t decay)
{
    attack_ = attack;
    decay_ = decay;
    segment_ = ENV_SEGMENT_ATTACK;
    phase_ = 0;
    phase_increment_ = ComputeIncrement(attack_);
    value_ = 0;
}

uint16_t Envelope::Render()
{
    if (segment_ == ENV_SEGMENT_DEAD) {
        return 0;
    }

    phase_ += phase_increment_;

    // Check for overflow (phase wrapped around)
    if (phase_ < phase_increment_) {
        // Phase completed
        if (segment_ == ENV_SEGMENT_ATTACK) {
            // Transition to decay
            segment_ = ENV_SEGMENT_DECAY;
            phase_ = 0;
            phase_increment_ = ComputeIncrement(decay_);
            value_ = 65535;
        } else {
            // Decay finished
            segment_ = ENV_SEGMENT_DEAD;
            value_ = 0;
            return 0;
        }
    }

    if (segment_ == ENV_SEGMENT_ATTACK) {
        // Linear rise from 0 to 65535
        value_ = static_cast<uint16_t>(phase_ >> 16);
    } else {
        // Linear fall from 65535 to 0
        value_ = static_cast<uint16_t>(65535 - (phase_ >> 16));
    }

    return value_;
}

} // namespace braids
