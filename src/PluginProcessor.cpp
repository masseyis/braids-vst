// BraidsVST - Mutable Instruments Braids port
// Original Braids: Copyright 2012 Emilie Gillet (MIT License)
// BraidsVST modifications: GPL v3

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/moog_filter.h"

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

    const juce::StringArray lfoRateNames = {
        "1/16", "1/16T", "1/8", "1/8T", "1/4", "1/4T", "1/2", "1", "2", "4"
    };

    const juce::StringArray lfoShapeNames = {
        "TRI", "SAW", "SQR", "S&H"
    };

    const juce::StringArray modDestNames = {
        "TIMBRE", "COLOR", "CUTOFF", "RESONAN", "LFO1 RT", "LFO1 AM", "LFO2 RT", "LFO2 AM"
    };
}

BraidsVSTProcessor::BraidsVSTProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , presetManager_(*this)
{
    // Create main synth parameters
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

    // Filter parameters
    addParameter(cutoffParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("cutoff", 1),
        "Cutoff",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        1.0f  // Default fully open
    ));

    addParameter(resonanceParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("resonance", 1),
        "Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f  // Default no resonance
    ));

    // LFO1 parameters
    addParameter(lfo1RateParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo1_rate", 1),
        "LFO1 Rate",
        lfoRateNames,
        4  // Default to 1/4
    ));

    addParameter(lfo1ShapeParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo1_shape", 1),
        "LFO1 Shape",
        lfoShapeNames,
        0  // Default to TRI
    ));

    addParameter(lfo1DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo1_dest", 1),
        "LFO1 Dest",
        modDestNames,
        0  // Default to TIMBRE
    ));

    addParameter(lfo1AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("lfo1_amount", 1),
        "LFO1 Amount",
        -64, 63, 0  // Bipolar, default off
    ));

    // LFO2 parameters
    addParameter(lfo2RateParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo2_rate", 1),
        "LFO2 Rate",
        lfoRateNames,
        4  // Default to 1/4
    ));

    addParameter(lfo2ShapeParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo2_shape", 1),
        "LFO2 Shape",
        lfoShapeNames,
        0  // Default to TRI
    ));

    addParameter(lfo2DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("lfo2_dest", 1),
        "LFO2 Dest",
        modDestNames,
        1  // Default to COLOR
    ));

    addParameter(lfo2AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("lfo2_amount", 1),
        "LFO2 Amount",
        -64, 63, 0  // Bipolar, default off
    ));

    // ENV1 parameters
    addParameter(env1AttackParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env1_attack", 1),
        "ENV1 Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.02f  // 10ms default
    ));

    addParameter(env1DecayParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env1_decay", 1),
        "ENV1 Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.1f  // ~200ms default
    ));

    addParameter(env1DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("env1_dest", 1),
        "ENV1 Dest",
        modDestNames,
        0  // Default to TIMBRE
    ));

    addParameter(env1AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("env1_amount", 1),
        "ENV1 Amount",
        -64, 63, 0  // Bipolar, default off
    ));

    // ENV2 parameters
    addParameter(env2AttackParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env2_attack", 1),
        "ENV2 Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.02f  // 10ms default
    ));

    addParameter(env2DecayParam_ = new juce::AudioParameterFloat(
        juce::ParameterID("env2_decay", 1),
        "ENV2 Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.1f  // ~200ms default
    ));

    addParameter(env2DestParam_ = new juce::AudioParameterChoice(
        juce::ParameterID("env2_dest", 1),
        "ENV2 Dest",
        modDestNames,
        1  // Default to COLOR
    ));

    addParameter(env2AmountParam_ = new juce::AudioParameterInt(
        juce::ParameterID("env2_amount", 1),
        "ENV2 Amount",
        -64, 63, 0  // Bipolar, default off
    ));

    voiceAllocator_.Init(44100.0, 8);
    modMatrix_.Init();
    filter_.Init(44100.0f);

    // Initialize preset manager after parameters are created
    presetManager_.initialize();
}

BraidsVSTProcessor::~BraidsVSTProcessor() = default;

void BraidsVSTProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate_ = sampleRate;
    voiceAllocator_.Init(sampleRate, polyphonyParam_->get());
    modMatrix_.Init();
    filter_.Init(static_cast<float>(sampleRate));
}

void BraidsVSTProcessor::releaseResources()
{
}

void BraidsVSTProcessor::updateModulationParams()
{
    // Update LFO1
    modMatrix_.GetLfo1().SetRate(static_cast<braids::LfoRateDivision>(lfo1RateParam_->getIndex()));
    modMatrix_.GetLfo1().SetShape(static_cast<braids::LfoShape>(lfo1ShapeParam_->getIndex()));
    modMatrix_.SetDestination(braids::ModSource::Lfo1, static_cast<braids::ModDestination>(lfo1DestParam_->getIndex()));
    modMatrix_.SetAmount(braids::ModSource::Lfo1, static_cast<int8_t>(lfo1AmountParam_->get()));

    // Update LFO2
    modMatrix_.GetLfo2().SetRate(static_cast<braids::LfoRateDivision>(lfo2RateParam_->getIndex()));
    modMatrix_.GetLfo2().SetShape(static_cast<braids::LfoShape>(lfo2ShapeParam_->getIndex()));
    modMatrix_.SetDestination(braids::ModSource::Lfo2, static_cast<braids::ModDestination>(lfo2DestParam_->getIndex()));
    modMatrix_.SetAmount(braids::ModSource::Lfo2, static_cast<int8_t>(lfo2AmountParam_->get()));

    // Update ENV1
    uint16_t env1Attack = static_cast<uint16_t>(env1AttackParam_->get() * 500.0f);
    uint16_t env1Decay = static_cast<uint16_t>(10.0f + env1DecayParam_->get() * 1990.0f);
    modMatrix_.GetEnv1().SetAttack(env1Attack);
    modMatrix_.GetEnv1().SetDecay(env1Decay);
    modMatrix_.SetDestination(braids::ModSource::Env1, static_cast<braids::ModDestination>(env1DestParam_->getIndex()));
    modMatrix_.SetAmount(braids::ModSource::Env1, static_cast<int8_t>(env1AmountParam_->get()));

    // Update ENV2
    uint16_t env2Attack = static_cast<uint16_t>(env2AttackParam_->get() * 500.0f);
    uint16_t env2Decay = static_cast<uint16_t>(10.0f + env2DecayParam_->get() * 1990.0f);
    modMatrix_.GetEnv2().SetAttack(env2Attack);
    modMatrix_.GetEnv2().SetDecay(env2Decay);
    modMatrix_.SetDestination(braids::ModSource::Env2, static_cast<braids::ModDestination>(env2DestParam_->getIndex()));
    modMatrix_.SetAmount(braids::ModSource::Env2, static_cast<int8_t>(env2AmountParam_->get()));
}

void BraidsVSTProcessor::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        // Check if this is the first note (for envelope triggering)
        int prevActiveCount = activeVoiceCount_;

        // Convert normalized parameters to actual milliseconds, then to uint16_t
        float attackMs = attackParam_->get() * 500.0f;
        float decayMs = 10.0f + decayParam_->get() * 1990.0f;
        uint16_t attack = static_cast<uint16_t>(attackMs);
        uint16_t decay = static_cast<uint16_t>(decayMs);
        voiceAllocator_.NoteOn(msg.getNoteNumber(), msg.getFloatVelocity(), attack, decay);

        activeVoiceCount_++;

        // Trigger mod envelopes on first note after silence
        if (prevActiveCount == 0) {
            modMatrix_.TriggerEnvelopes();
        }
    }
    else if (msg.isNoteOff())
    {
        voiceAllocator_.NoteOff(msg.getNoteNumber());
        if (activeVoiceCount_ > 0) {
            activeVoiceCount_--;
        }
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        voiceAllocator_.AllNotesOff();
        activeVoiceCount_ = 0;
    }
}

float BraidsVSTProcessor::getModulatedTimbre() const
{
    float base = timbreParam_->get();
    return modMatrix_.GetModulatedValue(braids::ModDestination::Timbre, base);
}

float BraidsVSTProcessor::getModulatedColor() const
{
    float base = colorParam_->get();
    return modMatrix_.GetModulatedValue(braids::ModDestination::Color, base);
}

float BraidsVSTProcessor::getModulatedCutoff() const
{
    float base = cutoffParam_->get();
    return modMatrix_.GetModulatedValue(braids::ModDestination::Cutoff, base);
}

float BraidsVSTProcessor::getModulatedResonance() const
{
    float base = resonanceParam_->get();
    return modMatrix_.GetModulatedValue(braids::ModDestination::Resonance, base);
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

    // Update modulation parameters from UI
    updateModulationParams();

    // Get host tempo if available, otherwise use default 120 BPM
    double tempo = 120.0;
    if (auto* playHead = getPlayHead()) {
        if (auto position = playHead->getPosition()) {
            if (auto bpm = position->getBpm()) {
                tempo = *bpm;
            }
        }
    }
    modMatrix_.SetTempo(tempo);

    // Process modulation matrix
    const int numSamples = buffer.getNumSamples();
    modMatrix_.Process(static_cast<float>(hostSampleRate_), numSamples);

    // Update shared parameters with modulation applied
    int shapeIndex = shapeParam_->getIndex();
    voiceAllocator_.set_shape(static_cast<braids::MacroOscillatorShape>(shapeIndex));

    // Apply modulation to timbre and color
    float modulatedTimbre = getModulatedTimbre();
    float modulatedColor = getModulatedColor();

    int16_t timbre = static_cast<int16_t>(modulatedTimbre * 32767.0f);
    int16_t color = static_cast<int16_t>(modulatedColor * 32767.0f);
    voiceAllocator_.set_parameters(timbre, color);

    // Get output pointers
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    // Process all voices
    if (rightChannel) {
        voiceAllocator_.Process(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    } else {
        // Mono output
        float tempRight[2048];
        size_t samples = std::min(static_cast<size_t>(numSamples), sizeof(tempRight) / sizeof(float));
        voiceAllocator_.Process(leftChannel, tempRight, samples);
    }

    // Update and apply filter with modulation
    float modulatedCutoff = getModulatedCutoff();
    float modulatedResonance = getModulatedResonance();

    // Convert normalized cutoff (0-1) to Hz (20-20000 using exponential scaling)
    float cutoffHz = 20.0f * std::pow(1000.0f, modulatedCutoff);
    filter_.SetCutoff(cutoffHz);
    filter_.SetResonance(modulatedResonance);

    // Apply filter to output (mono filter applied to both channels)
    for (int i = 0; i < numSamples; ++i) {
        // Mix L+R, filter, then split back (simple mono filter approach)
        float mono = leftChannel[i];
        if (rightChannel) {
            mono = (leftChannel[i] + rightChannel[i]) * 0.5f;
        }
        float filtered = filter_.Process(mono);
        leftChannel[i] = filtered;
        if (rightChannel) {
            rightChannel[i] = filtered;
        }
    }
}

juce::AudioProcessorEditor* BraidsVSTProcessor::createEditor()
{
    return new BraidsVSTEditor(*this);
}

void BraidsVSTProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = juce::ValueTree("BraidsVSTState");

    // Main params
    state.setProperty("shape", shapeParam_->getIndex(), nullptr);
    state.setProperty("timbre", timbreParam_->get(), nullptr);
    state.setProperty("color", colorParam_->get(), nullptr);
    state.setProperty("attack", attackParam_->get(), nullptr);
    state.setProperty("decay", decayParam_->get(), nullptr);
    state.setProperty("polyphony", polyphonyParam_->get(), nullptr);
    state.setProperty("cutoff", cutoffParam_->get(), nullptr);
    state.setProperty("resonance", resonanceParam_->get(), nullptr);

    // LFO1
    state.setProperty("lfo1_rate", lfo1RateParam_->getIndex(), nullptr);
    state.setProperty("lfo1_shape", lfo1ShapeParam_->getIndex(), nullptr);
    state.setProperty("lfo1_dest", lfo1DestParam_->getIndex(), nullptr);
    state.setProperty("lfo1_amount", lfo1AmountParam_->get(), nullptr);

    // LFO2
    state.setProperty("lfo2_rate", lfo2RateParam_->getIndex(), nullptr);
    state.setProperty("lfo2_shape", lfo2ShapeParam_->getIndex(), nullptr);
    state.setProperty("lfo2_dest", lfo2DestParam_->getIndex(), nullptr);
    state.setProperty("lfo2_amount", lfo2AmountParam_->get(), nullptr);

    // ENV1
    state.setProperty("env1_attack", env1AttackParam_->get(), nullptr);
    state.setProperty("env1_decay", env1DecayParam_->get(), nullptr);
    state.setProperty("env1_dest", env1DestParam_->getIndex(), nullptr);
    state.setProperty("env1_amount", env1AmountParam_->get(), nullptr);

    // ENV2
    state.setProperty("env2_attack", env2AttackParam_->get(), nullptr);
    state.setProperty("env2_decay", env2DecayParam_->get(), nullptr);
    state.setProperty("env2_dest", env2DestParam_->getIndex(), nullptr);
    state.setProperty("env2_amount", env2AmountParam_->get(), nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void BraidsVSTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        // Main params
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
        if (state.hasProperty("cutoff"))
            *cutoffParam_ = static_cast<float>(state.getProperty("cutoff"));
        if (state.hasProperty("resonance"))
            *resonanceParam_ = static_cast<float>(state.getProperty("resonance"));

        // LFO1
        if (state.hasProperty("lfo1_rate"))
            *lfo1RateParam_ = static_cast<int>(state.getProperty("lfo1_rate"));
        if (state.hasProperty("lfo1_shape"))
            *lfo1ShapeParam_ = static_cast<int>(state.getProperty("lfo1_shape"));
        if (state.hasProperty("lfo1_dest"))
            *lfo1DestParam_ = static_cast<int>(state.getProperty("lfo1_dest"));
        if (state.hasProperty("lfo1_amount"))
            *lfo1AmountParam_ = static_cast<int>(state.getProperty("lfo1_amount"));

        // LFO2
        if (state.hasProperty("lfo2_rate"))
            *lfo2RateParam_ = static_cast<int>(state.getProperty("lfo2_rate"));
        if (state.hasProperty("lfo2_shape"))
            *lfo2ShapeParam_ = static_cast<int>(state.getProperty("lfo2_shape"));
        if (state.hasProperty("lfo2_dest"))
            *lfo2DestParam_ = static_cast<int>(state.getProperty("lfo2_dest"));
        if (state.hasProperty("lfo2_amount"))
            *lfo2AmountParam_ = static_cast<int>(state.getProperty("lfo2_amount"));

        // ENV1
        if (state.hasProperty("env1_attack"))
            *env1AttackParam_ = static_cast<float>(state.getProperty("env1_attack"));
        if (state.hasProperty("env1_decay"))
            *env1DecayParam_ = static_cast<float>(state.getProperty("env1_decay"));
        if (state.hasProperty("env1_dest"))
            *env1DestParam_ = static_cast<int>(state.getProperty("env1_dest"));
        if (state.hasProperty("env1_amount"))
            *env1AmountParam_ = static_cast<int>(state.getProperty("env1_amount"));

        // ENV2
        if (state.hasProperty("env2_attack"))
            *env2AttackParam_ = static_cast<float>(state.getProperty("env2_attack"));
        if (state.hasProperty("env2_decay"))
            *env2DecayParam_ = static_cast<float>(state.getProperty("env2_decay"));
        if (state.hasProperty("env2_dest"))
            *env2DestParam_ = static_cast<int>(state.getProperty("env2_dest"));
        if (state.hasProperty("env2_amount"))
            *env2AmountParam_ = static_cast<int>(state.getProperty("env2_amount"));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraidsVSTProcessor();
}
