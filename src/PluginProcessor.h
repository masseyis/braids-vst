#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/voice_allocator.h"
#include "dsp/modulation_matrix.h"
#include "dsp/moog_filter.h"
#include "PresetManager.h"

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

    // Parameter accessors - Main synth params
    juce::AudioParameterChoice* getShapeParam() { return shapeParam_; }
    juce::AudioParameterFloat* getTimbreParam() { return timbreParam_; }
    juce::AudioParameterFloat* getColorParam() { return colorParam_; }
    juce::AudioParameterFloat* getAttackParam() { return attackParam_; }
    juce::AudioParameterFloat* getDecayParam() { return decayParam_; }
    juce::AudioParameterInt* getPolyphonyParam() { return polyphonyParam_; }

    // Filter params
    juce::AudioParameterFloat* getCutoffParam() { return cutoffParam_; }
    juce::AudioParameterFloat* getResonanceParam() { return resonanceParam_; }

    // LFO1 params
    juce::AudioParameterChoice* getLfo1RateParam() { return lfo1RateParam_; }
    juce::AudioParameterChoice* getLfo1ShapeParam() { return lfo1ShapeParam_; }
    juce::AudioParameterChoice* getLfo1DestParam() { return lfo1DestParam_; }
    juce::AudioParameterInt* getLfo1AmountParam() { return lfo1AmountParam_; }

    // LFO2 params
    juce::AudioParameterChoice* getLfo2RateParam() { return lfo2RateParam_; }
    juce::AudioParameterChoice* getLfo2ShapeParam() { return lfo2ShapeParam_; }
    juce::AudioParameterChoice* getLfo2DestParam() { return lfo2DestParam_; }
    juce::AudioParameterInt* getLfo2AmountParam() { return lfo2AmountParam_; }

    // ENV1 params
    juce::AudioParameterFloat* getEnv1AttackParam() { return env1AttackParam_; }
    juce::AudioParameterFloat* getEnv1DecayParam() { return env1DecayParam_; }
    juce::AudioParameterChoice* getEnv1DestParam() { return env1DestParam_; }
    juce::AudioParameterInt* getEnv1AmountParam() { return env1AmountParam_; }

    // ENV2 params
    juce::AudioParameterFloat* getEnv2AttackParam() { return env2AttackParam_; }
    juce::AudioParameterFloat* getEnv2DecayParam() { return env2DecayParam_; }
    juce::AudioParameterChoice* getEnv2DestParam() { return env2DestParam_; }
    juce::AudioParameterInt* getEnv2AmountParam() { return env2AmountParam_; }

    // Modulation matrix access for UI visualization
    const braids::ModulationMatrix& getModMatrix() const { return modMatrix_; }

    // Get current modulated values (for UI visualization)
    float getModulatedTimbre() const;
    float getModulatedColor() const;
    float getModulatedCutoff() const;
    float getModulatedResonance() const;

    // Preset manager
    PresetManager& getPresetManager() { return presetManager_; }

private:
    void handleMidiMessage(const juce::MidiMessage& msg);
    void updateModulationParams();

    VoiceAllocator voiceAllocator_;
    braids::ModulationMatrix modMatrix_;
    braids::MoogFilter filter_;
    double hostSampleRate_ = 44100.0;
    int activeVoiceCount_ = 0;  // Track active voices for envelope triggering

    // Main synth parameters
    juce::AudioParameterChoice* shapeParam_ = nullptr;
    juce::AudioParameterFloat* timbreParam_ = nullptr;
    juce::AudioParameterFloat* colorParam_ = nullptr;
    juce::AudioParameterFloat* attackParam_ = nullptr;
    juce::AudioParameterFloat* decayParam_ = nullptr;
    juce::AudioParameterInt* polyphonyParam_ = nullptr;

    // Filter parameters
    juce::AudioParameterFloat* cutoffParam_ = nullptr;
    juce::AudioParameterFloat* resonanceParam_ = nullptr;

    // LFO1 parameters
    juce::AudioParameterChoice* lfo1RateParam_ = nullptr;
    juce::AudioParameterChoice* lfo1ShapeParam_ = nullptr;
    juce::AudioParameterChoice* lfo1DestParam_ = nullptr;
    juce::AudioParameterInt* lfo1AmountParam_ = nullptr;

    // LFO2 parameters
    juce::AudioParameterChoice* lfo2RateParam_ = nullptr;
    juce::AudioParameterChoice* lfo2ShapeParam_ = nullptr;
    juce::AudioParameterChoice* lfo2DestParam_ = nullptr;
    juce::AudioParameterInt* lfo2AmountParam_ = nullptr;

    // ENV1 parameters
    juce::AudioParameterFloat* env1AttackParam_ = nullptr;
    juce::AudioParameterFloat* env1DecayParam_ = nullptr;
    juce::AudioParameterChoice* env1DestParam_ = nullptr;
    juce::AudioParameterInt* env1AmountParam_ = nullptr;

    // ENV2 parameters
    juce::AudioParameterFloat* env2AttackParam_ = nullptr;
    juce::AudioParameterFloat* env2DecayParam_ = nullptr;
    juce::AudioParameterChoice* env2DestParam_ = nullptr;
    juce::AudioParameterInt* env2AmountParam_ = nullptr;

    // Preset manager
    PresetManager presetManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTProcessor)
};
