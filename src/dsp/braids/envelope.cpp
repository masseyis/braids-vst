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

uint32_t Envelope::ComputeIncrement(uint16_t time_param)
{
    // Convert time parameter (0-65535) to phase increment
    // Higher time_param = faster envelope
    // We want the phase to complete in a reasonable time
    // At 96kHz sample rate, we want:
    // - time_param = 0: very slow (~10 seconds)
    // - time_param = 65535: very fast (~1ms)

    // Exponential mapping: increment = base * 2^(time_param / scale)
    // Simplified: use a linear-ish mapping for now

    if (time_param == 0) {
        return 100;  // Very slow
    }

    // Scale the increment exponentially
    // The increment determines how fast phase goes from 0 to 2^32
    // At 96kHz, 2^32 / 96000 = ~44739 samples for full sweep at increment=1

    // Use a simple exponential approximation
    uint32_t increment = static_cast<uint32_t>(time_param) << 8;

    // Add minimum to avoid zero
    increment += 1000;

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
