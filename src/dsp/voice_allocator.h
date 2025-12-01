// VoiceAllocator - manages polyphonic voice allocation
// BraidsVST: GPL v3

#pragma once

#include <array>
#include <cstdint>
#include "voice.h"

class VoiceAllocator {
public:
    static constexpr size_t kMaxVoices = 16;

    VoiceAllocator() = default;
    ~VoiceAllocator() = default;

    void Init(double hostSampleRate, int polyphony);

    void NoteOn(int note, float velocity, uint16_t attack, uint16_t decay);
    void NoteOff(int note);
    void AllNotesOff();

    // Process all voices and mix to stereo output
    void Process(float* leftOutput, float* rightOutput, size_t size);

    // Shared parameters for all voices
    void set_shape(braids::MacroOscillatorShape shape) { shape_ = shape; }
    void set_parameters(int16_t timbre, int16_t color) {
        timbre_ = timbre;
        color_ = color;
    }

    // Polyphony control
    void setPolyphony(int polyphony);
    int polyphony() const { return polyphony_; }

    // State queries
    int activeVoiceCount() const;

private:
    Voice* findFreeVoice();
    Voice* findVoiceForNote(int note);
    Voice* stealVoice();

    std::array<Voice, kMaxVoices> voices_;
    std::array<uint32_t, kMaxVoices> voiceAge_;  // For voice stealing
    uint32_t noteCounter_ = 0;

    int polyphony_ = 8;
    double hostSampleRate_ = 48000.0;

    // Shared parameters
    braids::MacroOscillatorShape shape_ = braids::MACRO_OSC_SHAPE_FM;
    int16_t timbre_ = 0;
    int16_t color_ = 0;
};
