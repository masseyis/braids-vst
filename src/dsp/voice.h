// Voice class - combines oscillator, envelope, and resampler
// BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include "braids/macro_oscillator.h"
#include "braids/envelope.h"
#include "resampler.h"

class Voice {
public:
    static constexpr double kInternalSampleRate = 96000.0;
    static constexpr size_t kInternalBlockSize = 24;

    Voice() = default;
    ~Voice() = default;

    void Init(double hostSampleRate);

    void NoteOn(int note, float velocity, uint16_t attack, uint16_t decay);
    void NoteOff();

    // Process and mix into output buffer (adds to existing content)
    void Process(float* output, size_t size);

    // Setters for shared parameters
    void set_shape(braids::MacroOscillatorShape shape) { shape_ = shape; }
    void set_parameters(int16_t timbre, int16_t color) {
        timbre_ = timbre;
        color_ = color;
    }

    // State queries
    bool active() const { return active_; }
    int note() const { return note_; }

private:
    braids::MacroOscillator oscillator_;
    braids::Envelope envelope_;
    Resampler resampler_;

    // Voice state
    bool active_ = false;
    int note_ = -1;
    float velocity_ = 0.0f;

    // Shared parameters (set before processing)
    braids::MacroOscillatorShape shape_ = braids::MACRO_OSC_SHAPE_FM;
    int16_t timbre_ = 0;
    int16_t color_ = 0;

    // Internal buffers
    int16_t internalBuffer_[kInternalBlockSize];
    uint8_t syncBuffer_[kInternalBlockSize] = {0};
    float resampledBuffer_[256];  // Larger buffer for resampled output

    double hostSampleRate_ = 48000.0;
};
