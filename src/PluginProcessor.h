#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/voice_allocator.h"

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
    juce::AudioParameterInt* getPolyphonyParam() { return polyphonyParam_; }

private:
    void handleMidiMessage(const juce::MidiMessage& msg);

    VoiceAllocator voiceAllocator_;
    double hostSampleRate_ = 44100.0;

    // Parameters
    juce::AudioParameterChoice* shapeParam_ = nullptr;
    juce::AudioParameterFloat* timbreParam_ = nullptr;
    juce::AudioParameterFloat* colorParam_ = nullptr;
    juce::AudioParameterFloat* attackParam_ = nullptr;
    juce::AudioParameterFloat* decayParam_ = nullptr;
    juce::AudioParameterInt* polyphonyParam_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTProcessor)
};
