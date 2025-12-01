#include "PluginProcessor.h"
#include "PluginEditor.h"

BraidsVSTProcessor::BraidsVSTProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    oscillator_.Init();
}

BraidsVSTProcessor::~BraidsVSTProcessor() = default;

void BraidsVSTProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate_ = sampleRate;
    oscillator_.Init();
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

    if (!noteOn_)
    {
        buffer.clear();
        return;
    }

    // Set FM parameters (moderate values for now)
    oscillator_.set_parameters(8192, 16384);

    // For Phase 1, we skip resampling and just render at host rate
    // This means the pitch will be off, but we'll hear sound
    // Proper 96kHz->host resampling comes in Phase 3

    for (int i = 0; i < numSamples; i += static_cast<int>(kInternalBlockSize))
    {
        size_t blockSize = std::min(static_cast<size_t>(numSamples - i), kInternalBlockSize);

        oscillator_.Render(internalBuffer_, blockSize);

        // Convert int16 to float and apply velocity
        for (size_t j = 0; j < blockSize; ++j)
        {
            float sample = static_cast<float>(internalBuffer_[j]) / 32768.0f;
            sample *= currentVelocity_;

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

void BraidsVSTProcessor::getStateInformation(juce::MemoryBlock& /*destData*/)
{
}

void BraidsVSTProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraidsVSTProcessor();
}
