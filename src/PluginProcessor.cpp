#include "PluginProcessor.h"
#include "PluginEditor.h"

BraidsVSTProcessor::BraidsVSTProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

BraidsVSTProcessor::~BraidsVSTProcessor() = default;

void BraidsVSTProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate_ = sampleRate;
}

void BraidsVSTProcessor::releaseResources()
{
}

void BraidsVSTProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output for now - we'll add synthesis in later tasks
    buffer.clear();
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
