#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class BraidsVSTEditor : public juce::AudioProcessorEditor,
                        public juce::Timer
{
public:
    explicit BraidsVSTEditor(BraidsVSTProcessor&);
    ~BraidsVSTEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // Timer callback for UI refresh
    void timerCallback() override;

private:
    // Row types: Preset is special, others map to parameters
    enum class RowType { Preset, Shape, Timbre, Color, Attack, Decay, Voices };
    static constexpr int kNumRows = 7;

    struct RowConfig {
        const char* label;
        RowType type;
        int minVal;
        int maxVal;
        int smallStep;
        int largeStep;
        const char* suffix;
    };

    static const RowConfig kRowConfigs[kNumRows];

    // Get/set values in display units
    int getDisplayValue(int row) const;
    void setDisplayValue(int row, int value);
    juce::String formatValue(int row) const;
    float normalizedValue(int row) const;

    // Navigation
    void adjustValue(int row, int delta);
    int rowAtY(int y) const;

    BraidsVSTProcessor& processor_;
    int selectedRow_ = 0;
    int dragStartValue_ = 0;
    int dragStartX_ = 0;

    // Shape names for display
    const juce::StringArray shapeNames_ = {
        "SAW", "MORPH", "SAW/SQ", "SIN/TRI", "BUZZ",
        "SQ+SUB", "SAW+SUB", "SQ SYNC", "SAW SYNC", "FM"
    };

    // Dynamic labels for Timbre/Color based on current shape
    // Format: {timbreLabel, colorLabel} for each shape
    const char* timbreLabels_[10] = {
        "SHAPE",    // SAW - waveshaping amount
        "MORPH",    // MORPH - triangle->saw->square->PWM
        "SAW VAR",  // SAW/SQ - variable saw shape
        "FOLD",     // SIN/TRI - fold amount (harmonics)
        "DENSITY",  // BUZZ - harmonic density
        "PWM",      // SQ+SUB - pulse width
        "SAW VAR",  // SAW+SUB - variable saw shape
        "RATIO",    // SQ SYNC - slave pitch ratio
        "RATIO",    // SAW SYNC - slave pitch ratio
        "MOD IDX",  // FM - modulation index
    };

    const char* colorLabels_[10] = {
        "BRIGHT",   // SAW - brightness/DC offset
        "DRIVE",    // MORPH - lowpass filter + overdrive
        "SAW/SQ",   // SAW/SQ - crossfade saw to square
        "SIN/TRI",  // SIN/TRI - crossfade sine to triangle fold
        "DETUNE",   // BUZZ - detune between oscillators
        "SUB MIX",  // SQ+SUB - sub oscillator mix
        "SUB MIX",  // SAW+SUB - sub oscillator mix
        "SHAPE",    // SQ SYNC - waveshaping amount
        "SHAPE",    // SAW SYNC - waveshaping amount
        "FEEDBK",   // FM - feedback
    };

    // Get dynamic label for a row
    juce::String getDynamicLabel(int row) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTEditor)
};
