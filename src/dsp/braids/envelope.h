// Ported from Mutable Instruments Braids envelope.h
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include "../stmlib/stmlib.h"

namespace braids {

enum EnvelopeSegment {
    ENV_SEGMENT_ATTACK,
    ENV_SEGMENT_DECAY,
    ENV_SEGMENT_DEAD
};

class Envelope {
public:
    Envelope() = default;
    ~Envelope() = default;

    void Init();
    void Trigger(uint16_t attack, uint16_t decay);
    uint16_t Render();
    bool done() const { return segment_ == ENV_SEGMENT_DEAD; }

    void set_attack(uint16_t attack) { attack_ = attack; }
    void set_decay(uint16_t decay) { decay_ = decay; }

private:
    uint32_t ComputeIncrement(uint16_t time_param);

    EnvelopeSegment segment_ = ENV_SEGMENT_DEAD;
    uint32_t phase_ = 0;
    uint32_t phase_increment_ = 0;
    uint16_t attack_ = 0;
    uint16_t decay_ = 0;
    uint16_t value_ = 0;

    DISALLOW_COPY_AND_ASSIGN(Envelope);
};

} // namespace braids
