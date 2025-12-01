#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class BraidsVSTEditor : public juce::AudioProcessorEditor
{
public:
    explicit BraidsVSTEditor(BraidsVSTProcessor&);
    ~BraidsVSTEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    BraidsVSTProcessor& processor_;

    // Shape selector
    juce::ComboBox shapeCombo_;
    juce::Label shapeLabel_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment_;

    // Knobs
    juce::Slider timbreSlider_;
    juce::Label timbreLabel_;

    juce::Slider colorSlider_;
    juce::Label colorLabel_;

    juce::Slider attackSlider_;
    juce::Label attackLabel_;

    juce::Slider decaySlider_;
    juce::Label decayLabel_;

    juce::Slider polyphonySlider_;
    juce::Label polyphonyLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTEditor)
};
