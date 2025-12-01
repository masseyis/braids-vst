#include "PresetManager.h"
#include "PluginProcessor.h"

const juce::StringArray PresetManager::kAdjectives = {
    "Neon", "Chrome", "Void", "Plasma", "Quantum",
    "Cyber", "Binary", "Static", "Flux", "Dark"
};

const juce::StringArray PresetManager::kNouns = {
    "Grid", "Pulse", "Signal", "Wave", "Core",
    "Drone", "Circuit", "Glitch", "Beam", "Echo"
};

PresetManager::PresetManager(BraidsVSTProcessor& processor)
    : processor_(processor)
    , rng_(static_cast<unsigned>(std::time(nullptr)))
{
}

void PresetManager::initialize()
{
    loadFactoryPresets();
    loadUserPresets();
    rebuildPresetList();

    if (!presets_.empty()) {
        loadPreset(0);
    }
}

juce::File PresetManager::getUserPresetsFolder()
{
    auto musicFolder = juce::File::getSpecialLocation(juce::File::userMusicDirectory);
    return musicFolder.getChildFile("BraidsVST").getChildFile("Presets");
}

void PresetManager::loadFactoryPresets()
{
    factoryPresets_.clear();

    // Factory presets are hardcoded for simplicity
    // Format: name, shape, timbre, color, attack, decay, voices
    struct FactoryPresetData {
        const char* name;
        int shape;
        float timbre;
        float color;
        float attack;
        float decay;
        int voices;
    };

    const FactoryPresetData factoryData[] = {
        {"Init",          9,  0.504f, 0.504f, 0.100f, 0.095f, 8},  // FM, 64, 64, 50ms, 200ms
        {"Neon Lead",     0,  0.787f, 0.315f, 0.010f, 0.146f, 4},  // Saw, 100, 40, 5ms, 300ms
        {"Chrome Bass",   5,  0.630f, 0.709f, 0.000f, 0.196f, 2},  // Square+Sub, 80, 90, 0ms, 400ms
        {"Void Pad",      1,  0.472f, 0.551f, 0.400f, 0.749f, 8},  // Morph, 60, 70, 200ms, 1500ms
        {"Plasma Pluck",  9,  0.709f, 0.394f, 0.000f, 0.070f, 6},  // FM, 90, 50, 0ms, 150ms
        {"Quantum Bell",  3,  0.866f, 0.236f, 0.000f, 0.397f, 8},  // Sine/Tri, 110, 30, 0ms, 800ms
        {"Cyber Sync",    8,  0.551f, 0.630f, 0.020f, 0.121f, 4},  // Saw Sync, 70, 80, 10ms, 250ms
        {"Binary Buzz",   4,  0.394f, 0.787f, 0.010f, 0.095f, 4},  // Buzz, 50, 100, 5ms, 200ms
        {"Static Drone",  6,  0.315f, 0.472f, 0.600f, 1.000f, 6},  // Saw+Sub, 40, 60, 300ms, 2000ms
        {"Flux Wave",     2,  0.669f, 0.354f, 0.040f, 0.246f, 4},  // Saw/Square, 85, 45, 20ms, 500ms
        {"Dark Core",     7,  0.236f, 0.945f, 0.100f, 0.296f, 2},  // Square Sync, 30, 120, 50ms, 600ms
        {"Glitch Signal", 9,  1.000f, 1.000f, 0.000f, 0.045f, 8},  // FM, 127, 127, 0ms, 100ms
    };

    for (const auto& data : factoryData) {
        Preset preset;
        preset.name = data.name;
        preset.isFactory = true;

        juce::ValueTree state("BraidsVSTState");
        state.setProperty("name", data.name, nullptr);
        state.setProperty("shape", data.shape, nullptr);
        state.setProperty("timbre", data.timbre, nullptr);
        state.setProperty("color", data.color, nullptr);
        state.setProperty("attack", data.attack, nullptr);
        state.setProperty("decay", data.decay, nullptr);
        state.setProperty("polyphony", data.voices, nullptr);
        preset.state = state;

        factoryPresets_.push_back(preset);
    }
}

void PresetManager::loadUserPresets()
{
    userPresets_.clear();

    auto folder = getUserPresetsFolder();
    if (!folder.exists()) {
        return;
    }

    auto files = folder.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (const auto& file : files) {
        auto preset = loadPresetFromFile(file, false);
        if (preset.state.isValid()) {
            userPresets_.push_back(preset);
        }
    }
}

PresetManager::Preset PresetManager::loadPresetFromFile(const juce::File& file, bool isFactory)
{
    Preset preset;
    preset.file = file;
    preset.isFactory = isFactory;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr) {
        preset.state = juce::ValueTree::fromXml(*xml);
        if (preset.state.hasProperty("name")) {
            preset.name = preset.state.getProperty("name").toString();
        } else {
            preset.name = file.getFileNameWithoutExtension();
        }
    }

    return preset;
}

void PresetManager::rebuildPresetList()
{
    presets_.clear();

    // Add factory presets first
    for (const auto& p : factoryPresets_) {
        presets_.push_back(p);
    }

    // Add user presets
    for (const auto& p : userPresets_) {
        presets_.push_back(p);
    }

    // Sort alphabetically (factory first by virtue of typical naming, but mixed)
    std::sort(presets_.begin(), presets_.end(),
              [](const Preset& a, const Preset& b) {
                  // Factory presets come first
                  if (a.isFactory != b.isFactory) {
                      return a.isFactory;
                  }
                  return a.name.compareIgnoreCase(b.name) < 0;
              });
}

void PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets_.size())) {
        return;
    }

    currentPresetIndex_ = index;
    applyPresetToProcessor(presets_[static_cast<size_t>(index)]);
    isModified_ = false;
}

void PresetManager::applyPresetToProcessor(const Preset& preset)
{
    const auto& state = preset.state;

    if (state.hasProperty("shape")) {
        *processor_.getShapeParam() = static_cast<int>(state.getProperty("shape"));
    }
    if (state.hasProperty("timbre")) {
        *processor_.getTimbreParam() = static_cast<float>(state.getProperty("timbre"));
    }
    if (state.hasProperty("color")) {
        *processor_.getColorParam() = static_cast<float>(state.getProperty("color"));
    }
    if (state.hasProperty("attack")) {
        *processor_.getAttackParam() = static_cast<float>(state.getProperty("attack"));
    }
    if (state.hasProperty("decay")) {
        *processor_.getDecayParam() = static_cast<float>(state.getProperty("decay"));
    }
    if (state.hasProperty("polyphony")) {
        *processor_.getPolyphonyParam() = static_cast<int>(state.getProperty("polyphony"));
    }
}

void PresetManager::nextPreset()
{
    if (presets_.empty()) return;
    loadPreset((currentPresetIndex_ + 1) % static_cast<int>(presets_.size()));
}

void PresetManager::previousPreset()
{
    if (presets_.empty()) return;
    int newIndex = currentPresetIndex_ - 1;
    if (newIndex < 0) {
        newIndex = static_cast<int>(presets_.size()) - 1;
    }
    loadPreset(newIndex);
}

juce::ValueTree PresetManager::captureCurrentState()
{
    juce::ValueTree state("BraidsVSTState");
    state.setProperty("shape", processor_.getShapeParam()->getIndex(), nullptr);
    state.setProperty("timbre", processor_.getTimbreParam()->get(), nullptr);
    state.setProperty("color", processor_.getColorParam()->get(), nullptr);
    state.setProperty("attack", processor_.getAttackParam()->get(), nullptr);
    state.setProperty("decay", processor_.getDecayParam()->get(), nullptr);
    state.setProperty("polyphony", processor_.getPolyphonyParam()->get(), nullptr);
    return state;
}

juce::String PresetManager::generateRandomName()
{
    std::uniform_int_distribution<int> adjDist(0, kAdjectives.size() - 1);
    std::uniform_int_distribution<int> nounDist(0, kNouns.size() - 1);

    return kAdjectives[adjDist(rng_)] + " " + kNouns[nounDist(rng_)];
}

void PresetManager::saveCurrentAsNewPreset()
{
    auto folder = getUserPresetsFolder();
    if (!folder.exists()) {
        folder.createDirectory();
    }

    // Generate a unique name
    juce::String baseName = generateRandomName();
    juce::String name = baseName;
    juce::File file = folder.getChildFile(name + ".xml");

    int suffix = 2;
    while (file.exists()) {
        name = baseName + " " + juce::String(suffix);
        file = folder.getChildFile(name + ".xml");
        suffix++;
    }

    // Capture current state
    auto state = captureCurrentState();
    state.setProperty("name", name, nullptr);

    // Write to file
    auto xml = state.createXml();
    if (xml != nullptr) {
        xml->writeTo(file);
    }

    // Add to user presets and rebuild
    Preset preset;
    preset.name = name;
    preset.file = file;
    preset.isFactory = false;
    preset.state = state;
    userPresets_.push_back(preset);

    rebuildPresetList();

    // Find and select the new preset
    for (int i = 0; i < static_cast<int>(presets_.size()); ++i) {
        if (presets_[static_cast<size_t>(i)].name == name) {
            currentPresetIndex_ = i;
            break;
        }
    }

    isModified_ = false;
}

juce::String PresetManager::getCurrentPresetName() const
{
    if (presets_.empty()) {
        return "No Presets";
    }

    juce::String name = presets_[static_cast<size_t>(currentPresetIndex_)].name;
    if (isModified_) {
        name += " *";
    }
    return name;
}
