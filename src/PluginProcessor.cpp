// BraidsVST - Mutable Instruments Braids port
// Original Braids: Copyright 2012 Emilie Gillet (MIT License)
// BraidsVST modifications: GPL v3

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
    const juce::StringArray shapeNames = {
        "Saw",              // CSAW
        "Morph",            // MORPH
        "Saw/Square",       // SAW_SQUARE
        "Sine/Triangle",    // SINE_TRIANGLE
        "Buzz",             // BUZZ
        "Square+Sub",       // SQUARE_SUB
        "Saw+Sub",          // SAW_SUB
        "Square Sync",      // SQUARE_SYNC
        "Saw Sync",         // SAW_SYNC
        "FM"                // FM
    };
}

BraidsVSTProcessor::BraidsVSTProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Create parameters
    addParameter(shapeParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("shape", 1),
        "Shape",
        shapeNames,
        9  // Default to FM
    ));

    addParameter(timbreParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("timbre", 1),
        "Timbre",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));

    addParameter(colorParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("color", 1),
        "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f
    ));

    addParameter(attackParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("attack", 1),
        "Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.3f),  // Skewed for finer control at low values
        0.1f
    ));

    addParameter(decayParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("decay", 1),
        "Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.3f),
        0.3f
    ));

    oscillator_.Init();
    envelope_.Init();
}

BraidsVSTProcessor::~BraidsVSTProcessor() = default;

void BraidsVSTProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate_ = sampleRate;
    oscillator_.Init();
    envelope_.Init();
}

void BraidsVSTProcessor::releaseResources()
{
}

void BraidsVSTProcessor::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        noteOn_ = true;
        currentNote_ = msg.getNoteNumber();
        currentVelocity_ = msg.getFloatVelocity();

        // Convert MIDI note to Braids pitch format (note * 128)
        oscillator_.set_pitch(currentNote_ << 7);

        // Trigger envelope with current attack/decay settings
        uint16_t attack = static_cast<uint16_t>(attackParam_->get() * 65535.0f);
        uint16_t decay = static_cast<uint16_t>(decayParam_->get() * 65535.0f);
        envelope_.Trigger(attack, decay);
    }
    else if (msg.isNoteOff() && msg.getNoteNumber() == currentNote_)
    {
        noteOn_ = false;
    }
}

void BraidsVSTProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Handle MIDI messages
    for (const auto metadata : midiMessages)
    {
        handleMidiMessage(metadata.getMessage());
    }

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    const int numSamples = buffer.getNumSamples();

    // Update shape from parameter
    int shapeIndex = shapeParam_->getIndex();
    oscillator_.set_shape(static_cast<braids::MacroOscillatorShape>(shapeIndex));

    // Update timbre/color parameters (scaled to Braids 0-32767 range)
    int16_t timbre = static_cast<int16_t>(timbreParam_->get() * 32767.0f);
    int16_t color = static_cast<int16_t>(colorParam_->get() * 32767.0f);
    oscillator_.set_parameters(timbre, color);

    // If no note playing and envelope done, clear buffer
    if (!noteOn_ && envelope_.done())
    {
        buffer.clear();
        return;
    }

    for (int i = 0; i < numSamples; i += static_cast<int>(kInternalBlockSize))
    {
        size_t blockSize = std::min(static_cast<size_t>(numSamples - i), kInternalBlockSize);

        oscillator_.Render(syncBuffer_, internalBuffer_, blockSize);

        // Convert int16 to float, apply envelope and velocity
        for (size_t j = 0; j < blockSize; ++j)
        {
            // Get envelope value (0-65535) and scale to 0.0-1.0
            uint16_t envValue = envelope_.Render();
            float envGain = static_cast<float>(envValue) / 65535.0f;

            float sample = static_cast<float>(internalBuffer_[j]) / 32768.0f;
            sample *= envGain * currentVelocity_;

            leftChannel[i + static_cast<int>(j)] = sample;
            if (rightChannel)
                rightChannel[i + static_cast<int>(j)] = sample;
        }
    }
}

juce::AudioProcessorEditor* BraidsVSTProcessor::createEditor()
{
    return new BraidsVSTEditor(*this);
}

void BraidsVSTProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameters
    auto state = juce::ValueTree("BraidsVSTState");
    state.setProperty("shape", shapeParam_->getIndex(), nullptr);
    state.setProperty("timbre", timbreParam_->get(), nullptr);
    state.setProperty("color", colorParam_->get(), nullptr);
    state.setProperty("attack", attackParam_->get(), nullptr);
    state.setProperty("decay", decayParam_->get(), nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void BraidsVSTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Load parameters
    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        if (state.hasProperty("shape"))
            *shapeParam_ = static_cast<int>(state.getProperty("shape"));
        if (state.hasProperty("timbre"))
            *timbreParam_ = static_cast<float>(state.getProperty("timbre"));
        if (state.hasProperty("color"))
            *colorParam_ = static_cast<float>(state.getProperty("color"));
        if (state.hasProperty("attack"))
            *attackParam_ = static_cast<float>(state.getProperty("attack"));
        if (state.hasProperty("decay"))
            *decayParam_ = static_cast<float>(state.getProperty("decay"));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraidsVSTProcessor();
}
