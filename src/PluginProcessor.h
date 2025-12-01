#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/braids/macro_oscillator.h"
#include "dsp/braids/envelope.h"

class BraidsVSTProcessor : public juce::AudioProcessor
{
public:
    BraidsVSTProcessor();
    ~BraidsVSTProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter accessors
    juce::AudioParameterChoice* getShapeParam() { return shapeParam_; }
    juce::AudioParameterFloat* getTimbreParam() { return timbreParam_; }
    juce::AudioParameterFloat* getColorParam() { return colorParam_; }
    juce::AudioParameterFloat* getAttackParam() { return attackParam_; }
    juce::AudioParameterFloat* getDecayParam() { return decayParam_; }

private:
    void handleMidiMessage(const juce::MidiMessage& msg);

    braids::MacroOscillator oscillator_;
    braids::Envelope envelope_;
    double hostSampleRate_ = 44100.0;

    // Parameters
    juce::AudioParameterChoice* shapeParam_ = nullptr;
    juce::AudioParameterFloat* timbreParam_ = nullptr;
    juce::AudioParameterFloat* colorParam_ = nullptr;
    juce::AudioParameterFloat* attackParam_ = nullptr;
    juce::AudioParameterFloat* decayParam_ = nullptr;

    // Simple voice state
    bool noteOn_ = false;
    int currentNote_ = 60;
    float currentVelocity_ = 0.0f;

    // Internal 96kHz buffer for fixed-point rendering
    static constexpr size_t kInternalBlockSize = 24;
    int16_t internalBuffer_[kInternalBlockSize];
    uint8_t syncBuffer_[kInternalBlockSize] = {0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTProcessor)
};
