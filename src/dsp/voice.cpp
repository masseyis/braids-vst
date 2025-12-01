// Voice class - combines oscillator, envelope, and resampler
// BraidsVST: GPL v3

#include "voice.h"
#include <algorithm>
#include <cstring>

void Voice::Init(double hostSampleRate)
{
    hostSampleRate_ = hostSampleRate;
    oscillator_.Init();
    envelope_.Init();
    resampler_.Init(kInternalSampleRate, hostSampleRate);
    active_ = false;
    note_ = -1;
    velocity_ = 0.0f;
}

void Voice::NoteOn(int note, float velocity, uint16_t attack, uint16_t decay)
{
    note_ = note;
    velocity_ = velocity;
    active_ = true;

    // Reset oscillator phase for consistent attack
    oscillator_.Init();

    // Set pitch: Braids uses note * 128
    oscillator_.set_pitch(static_cast<int16_t>(note << 7));

    // Trigger envelope
    envelope_.Trigger(attack, decay);

    // Reset resampler for clean start
    resampler_.Reset();
}

void Voice::NoteOff()
{
    // For AD envelope, note off doesn't do anything special
    // Voice becomes inactive when envelope finishes
}

void Voice::Process(float* output, size_t size)
{
    if (!active_) {
        return;
    }

    // Update oscillator parameters
    oscillator_.set_shape(shape_);
    oscillator_.set_parameters(timbre_, color_);

    size_t outputWritten = 0;

    while (outputWritten < size) {
        // Calculate how many 96kHz samples we need to generate
        // to produce the remaining output samples
        size_t outputRemaining = size - outputWritten;

        // Generate internal samples at 96kHz
        // We need approximately (outputRemaining * ratio) input samples
        // Process in blocks of kInternalBlockSize
        size_t internalSamples = std::min(kInternalBlockSize,
            static_cast<size_t>(outputRemaining * resampler_.ratio()) + 4);

        oscillator_.Render(syncBuffer_, internalBuffer_, internalSamples);

        // Apply envelope to internal buffer
        for (size_t i = 0; i < internalSamples; ++i) {
            uint16_t envValue = envelope_.Render();
            float envGain = static_cast<float>(envValue) / 65535.0f;

            // Apply envelope and velocity
            float sample = static_cast<float>(internalBuffer_[i]) / 32768.0f;
            sample *= envGain * velocity_;
            internalBuffer_[i] = static_cast<int16_t>(sample * 32767.0f);
        }

        // Check if envelope finished
        if (envelope_.done()) {
            active_ = false;
        }

        // Resample to host rate
        size_t maxOutput = std::min(outputRemaining, sizeof(resampledBuffer_) / sizeof(float));
        size_t produced = resampler_.Process(internalBuffer_, internalSamples,
                                              resampledBuffer_, maxOutput);

        // Mix into output (add to existing content)
        for (size_t i = 0; i < produced; ++i) {
            output[outputWritten + i] += resampledBuffer_[i];
        }

        outputWritten += produced;

        // Safety check to prevent infinite loop
        if (produced == 0 && internalSamples > 0) {
            break;
        }
    }
}
