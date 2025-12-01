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
    , presetManager_(*this)
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

    // Attack: 0-500ms mapped to 0-1
    addParameter(attackParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("attack", 1),
        "Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.1f  // 50ms default
    ));

    // Decay: 10-2000ms mapped to 0-1
    addParameter(decayParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("decay", 1),
        "Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.095f  // ~200ms default: (200-10)/1990 = 0.095
    ));

    addParameter(polyphonyParam_ = new juce::AudioParameterInt(
        juce::ParameterID("polyphony", 1),
        "Polyphony",
        1, 16, 8  // min, max, default
    ));

    voiceAllocator_.Init(44100.0, 8);

    // Initialize preset manager after parameters are created
    presetManager_.initialize();
}

BraidsVSTProcessor::~BraidsVSTProcessor() = default;

void BraidsVSTProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate_ = sampleRate;
    voiceAllocator_.Init(sampleRate, polyphonyParam_->get());
}

void BraidsVSTProcessor::releaseResources()
{
}

void BraidsVSTProcessor::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        // Convert normalized parameters to actual milliseconds, then to uint16_t
        // Attack: 0.0-1.0 → 0-500ms → 0-500 stored in uint16_t
        // Decay: 0.0-1.0 → 10-2000ms → 10-2000 stored in uint16_t
        // The envelope's ComputeIncrement interprets these as milliseconds directly
        float attackMs = attackParam_->get() * 500.0f;
        float decayMs = 10.0f + decayParam_->get() * 1990.0f;
        uint16_t attack = static_cast<uint16_t>(attackMs);
        uint16_t decay = static_cast<uint16_t>(decayMs);
        voiceAllocator_.NoteOn(msg.getNoteNumber(), msg.getFloatVelocity(), attack, decay);
    }
    else if (msg.isNoteOff())
    {
        voiceAllocator_.NoteOff(msg.getNoteNumber());
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        voiceAllocator_.AllNotesOff();
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

    // Update polyphony if changed
    voiceAllocator_.setPolyphony(polyphonyParam_->get());

    // Update shared parameters
    int shapeIndex = shapeParam_->getIndex();
    voiceAllocator_.set_shape(static_cast<braids::MacroOscillatorShape>(shapeIndex));

    int16_t timbre = static_cast<int16_t>(timbreParam_->get() * 32767.0f);
    int16_t color = static_cast<int16_t>(colorParam_->get() * 32767.0f);
    voiceAllocator_.set_parameters(timbre, color);

    // Get output pointers
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    const int numSamples = buffer.getNumSamples();

    // Process all voices
    if (rightChannel) {
        voiceAllocator_.Process(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    } else {
        // Mono output
        float tempRight[2048];
        size_t samples = std::min(static_cast<size_t>(numSamples), sizeof(tempRight) / sizeof(float));
        voiceAllocator_.Process(leftChannel, tempRight, samples);
    }
}

juce::AudioProcessorEditor* BraidsVSTProcessor::createEditor()
{
    return new BraidsVSTEditor(*this);
}

void BraidsVSTProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = juce::ValueTree("BraidsVSTState");
    state.setProperty("shape", shapeParam_->getIndex(), nullptr);
    state.setProperty("timbre", timbreParam_->get(), nullptr);
    state.setProperty("color", colorParam_->get(), nullptr);
    state.setProperty("attack", attackParam_->get(), nullptr);
    state.setProperty("decay", decayParam_->get(), nullptr);
    state.setProperty("polyphony", polyphonyParam_->get(), nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void BraidsVSTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
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
        if (state.hasProperty("polyphony"))
            *polyphonyParam_ = static_cast<int>(state.getProperty("polyphony"));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraidsVSTProcessor();
}
