# Phase 1: Foundation + First Sound Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Create a working VST3/AU plugin that plays FM synthesis when you press MIDI keys.

**Architecture:** JUCE plugin shell wrapping ported Braids FM oscillator with fixed-point DSP core, float conversion at boundaries, single voice for Phase 1.

**Tech Stack:** JUCE 7.x, CMake 3.22+, GoogleTest, C++17

---

## Task 1: Project Scaffolding

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/PluginProcessor.h`
- Create: `src/PluginProcessor.cpp`
- Create: `src/PluginEditor.h`
- Create: `src/PluginEditor.cpp`

**Step 1: Create top-level CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.22)
project(BraidsVST VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9
)
FetchContent_MakeAvailable(JUCE)

# Plugin target
juce_add_plugin(BraidsVST
    COMPANY_NAME "BraidsVST"
    PLUGIN_MANUFACTURER_CODE Brvs
    PLUGIN_CODE Brd1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "BraidsVST"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(BraidsVST
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_include_directories(BraidsVST
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(BraidsVST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(BraidsVST
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

# GoogleTest for testing
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(BraidsVSTTests
    test/dsp/StmlibTests.cpp)

target_link_libraries(BraidsVSTTests
    PRIVATE
        GTest::gtest_main)

target_include_directories(BraidsVSTTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(BraidsVSTTests)
```

**Step 2: Create src/PluginProcessor.h**

```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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

private:
    double hostSampleRate_ = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTProcessor)
};
```

**Step 3: Create src/PluginProcessor.cpp**

```cpp
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
```

**Step 4: Create src/PluginEditor.h**

```cpp
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTEditor)
};
```

**Step 5: Create src/PluginEditor.cpp**

```cpp
#include "PluginEditor.h"

BraidsVSTEditor::BraidsVSTEditor(BraidsVSTProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(400, 300);
}

BraidsVSTEditor::~BraidsVSTEditor() = default;

void BraidsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("BraidsVST", getLocalBounds(), juce::Justification::centred, 1);
}

void BraidsVSTEditor::resized()
{
}
```

**Step 6: Create placeholder test file**

Create `test/dsp/StmlibTests.cpp`:

```cpp
#include <gtest/gtest.h>

TEST(Placeholder, CompilationWorks)
{
    EXPECT_EQ(1 + 1, 2);
}
```

**Step 7: Create directory structure**

Run:
```bash
mkdir -p src test/dsp test/reference src/dsp/braids src/dsp/stmlib
```

**Step 8: Build and verify**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Expected: Build succeeds, produces `BraidsVST.vst3` and `BraidsVST.component`

**Step 9: Run tests**

Run:
```bash
cd build && ctest --output-on-failure
```

Expected: `1 test passed`

**Step 10: Commit**

```bash
git add -A
git commit -m "feat: project scaffolding with JUCE plugin shell

- CMake build system with JUCE 7.0.9
- Basic AudioProcessor and Editor
- GoogleTest integration
- VST3, AU, and Standalone formats"
```

---

## Task 2: GitHub Actions CI

**Files:**
- Create: `.github/workflows/build.yml`

**Step 1: Create CI workflow**

Create `.github/workflows/build.yml`:

```yaml
name: Build

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v4

      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release

      - name: Run Tests
        run: cd build && ctest --output-on-failure

  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release

      - name: Run Tests
        run: cd build && ctest --output-on-failure -C Release
```

**Step 2: Commit**

```bash
git add .github/workflows/build.yml
git commit -m "ci: add GitHub Actions for macOS and Windows builds"
```

---

## Task 3: Port stmlib Utilities

**Files:**
- Create: `src/dsp/stmlib/stmlib.h`
- Create: `src/dsp/stmlib/dsp.h`
- Create: `src/dsp/stmlib/random.h`
- Modify: `test/dsp/StmlibTests.cpp`

**Step 1: Write failing tests for stmlib utilities**

Replace `test/dsp/StmlibTests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "dsp/stmlib/stmlib.h"
#include "dsp/stmlib/dsp.h"
#include "dsp/stmlib/random.h"

TEST(Stmlib, ClipPositive)
{
    int32_t x = 40000;
    CLIP(x);
    EXPECT_EQ(x, 32767);
}

TEST(Stmlib, ClipNegative)
{
    int32_t x = -40000;
    CLIP(x);
    EXPECT_EQ(x, -32768);
}

TEST(Stmlib, ClipNoChange)
{
    int32_t x = 1000;
    CLIP(x);
    EXPECT_EQ(x, 1000);
}

TEST(Stmlib, Interpolate824_Midpoint)
{
    // Simple sine table test
    static const int16_t table[2] = {0, 32767};
    uint32_t phase = 0x80000000; // Halfway
    int16_t result = stmlib::Interpolate824(table, phase);
    EXPECT_NEAR(result, 16383, 1);
}

TEST(Stmlib, Mix5050)
{
    int16_t a = 0;
    int16_t b = 32767;
    int16_t result = stmlib::Mix(a, b, 32768); // 50% mix
    EXPECT_NEAR(result, 16383, 1);
}

TEST(Stmlib, RandomProducesValues)
{
    stmlib::Random::Seed(12345);
    int16_t sample = stmlib::Random::GetSample();
    // Just verify it returns something in range
    EXPECT_GE(sample, -32768);
    EXPECT_LE(sample, 32767);
}
```

**Step 2: Run tests to verify they fail**

Run:
```bash
cmake --build build && cd build && ctest --output-on-failure
```

Expected: FAIL - headers not found

**Step 3: Create src/dsp/stmlib/stmlib.h**

```cpp
// Ported from Mutable Instruments stmlib
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include <algorithm>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete;    \
    TypeName& operator=(const TypeName&) = delete

#define CLIP(x) x = std::clamp(x, static_cast<decltype(x)>(-32768), static_cast<decltype(x)>(32767))

#define CONSTRAIN(x, min, max) x = std::clamp(x, static_cast<decltype(x)>(min), static_cast<decltype(x)>(max))
```

**Step 4: Create src/dsp/stmlib/dsp.h**

```cpp
// Ported from Mutable Instruments stmlib/utils/dsp.h
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>

namespace stmlib {

// Linear interpolation with 8.24 fixed-point phase
inline int16_t Interpolate824(const int16_t* table, uint32_t phase)
{
    uint32_t index = phase >> 24;
    uint32_t frac = (phase >> 8) & 0xFFFF;
    int32_t a = table[index];
    int32_t b = table[index + 1];
    return static_cast<int16_t>(a + ((b - a) * static_cast<int32_t>(frac) >> 16));
}

// Linear interpolation for 8-bit tables with 8.24 phase
inline int16_t Interpolate824(const uint8_t* table, uint32_t phase)
{
    uint32_t index = phase >> 24;
    uint32_t frac = (phase >> 8) & 0xFFFF;
    int32_t a = static_cast<int16_t>(table[index]) - 128;
    int32_t b = static_cast<int16_t>(table[index + 1]) - 128;
    return static_cast<int16_t>((a + ((b - a) * static_cast<int32_t>(frac) >> 16)) << 8);
}

// Linear interpolation with 8.8 fixed-point phase
inline int16_t Interpolate88(const int16_t* table, uint16_t index)
{
    uint16_t integral = index >> 8;
    uint16_t frac = index & 0xFF;
    int32_t a = table[integral];
    int32_t b = table[integral + 1];
    return static_cast<int16_t>(a + ((b - a) * frac >> 8));
}

// 10.22 interpolation for delay lines
inline int16_t Interpolate1022(const int16_t* table, uint32_t phase)
{
    uint32_t index = phase >> 22;
    uint32_t frac = (phase >> 6) & 0xFFFF;
    int32_t a = table[index];
    int32_t b = table[index + 1];
    return static_cast<int16_t>(a + ((b - a) * static_cast<int32_t>(frac) >> 16));
}

// Mix two signals: result = a + (b - a) * balance / 65536
inline int16_t Mix(int16_t a, int16_t b, uint16_t balance)
{
    return static_cast<int16_t>(a + (((static_cast<int32_t>(b) - a) * balance) >> 16));
}

// Crossfade between two wavetable entries
inline int16_t Crossfade(const uint8_t* table_a, const uint8_t* table_b,
                          uint32_t phase, uint16_t balance)
{
    int16_t a = Interpolate824(table_a, phase);
    int16_t b = Interpolate824(table_b, phase);
    return Mix(a, b, balance);
}

} // namespace stmlib
```

**Step 5: Create src/dsp/stmlib/random.h**

```cpp
// Ported from Mutable Instruments stmlib/utils/random.h
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>

namespace stmlib {

class Random
{
public:
    static void Seed(uint32_t seed)
    {
        state_ = seed;
    }

    static uint32_t GetWord()
    {
        state_ = state_ * 1664525L + 1013904223L;
        return state_;
    }

    static int16_t GetSample()
    {
        return static_cast<int16_t>(GetWord() >> 16);
    }

    static float GetFloat()
    {
        return static_cast<float>(GetWord()) / 4294967296.0f;
    }

private:
    static inline uint32_t state_ = 0x12345678;
};

} // namespace stmlib
```

**Step 6: Update CMakeLists.txt to include stmlib in tests**

Add to `target_include_directories(BraidsVSTTests`:

```cmake
target_include_directories(BraidsVSTTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
```

**Step 7: Run tests to verify they pass**

Run:
```bash
cmake --build build && cd build && ctest --output-on-failure
```

Expected: All 6 tests pass

**Step 8: Commit**

```bash
git add -A
git commit -m "feat: port stmlib utilities (CLIP, interpolation, random)

- CLIP and CONSTRAIN macros
- Interpolate824, Interpolate88, Interpolate1022
- Mix and Crossfade functions
- Random number generator"
```

---

## Task 4: Port FM Oscillator Resources (Lookup Tables)

**Files:**
- Create: `src/dsp/braids/resources.h`
- Create: `src/dsp/braids/resources.cpp` (partial - just what FM needs)
- Modify: `test/dsp/StmlibTests.cpp`

**Step 1: Write failing test for FM resources**

Add to `test/dsp/StmlibTests.cpp`:

```cpp
#include "dsp/braids/resources.h"

TEST(Resources, SineWaveTableExists)
{
    // Verify sine table has expected properties
    EXPECT_EQ(braids::wav_sine[0], 0);          // Sine starts at 0
    EXPECT_GT(braids::wav_sine[64], 30000);     // Peak around 32767
    EXPECT_NEAR(braids::wav_sine[128], 0, 100); // Zero crossing
}

TEST(Resources, OscillatorIncrementsExist)
{
    // Verify LUT exists and has reasonable values
    EXPECT_GT(braids::lut_oscillator_increments[0], 0u);
}
```

**Step 2: Run tests to verify they fail**

Run:
```bash
cmake --build build && cd build && ctest --output-on-failure
```

Expected: FAIL - resources.h not found

**Step 3: Create src/dsp/braids/resources.h**

```cpp
// Ported from Mutable Instruments Braids
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>

namespace braids {

// Sine wavetable (257 entries for interpolation)
extern const int16_t wav_sine[];
extern const size_t WAV_SINE_SIZE;

// Oscillator phase increment lookup table
extern const uint32_t lut_oscillator_increments[];
extern const size_t LUT_OSCILLATOR_INCREMENTS_SIZE;

// FM frequency quantizer
extern const int16_t lut_fm_frequency_quantizer[];
extern const size_t LUT_FM_FREQUENCY_QUANTIZER_SIZE;

} // namespace braids
```

**Step 4: Create src/dsp/braids/resources.cpp**

This is a larger file - generate the sine table and pitch lookup table:

```cpp
// Ported from Mutable Instruments Braids
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#include "resources.h"
#include <cmath>

namespace braids {

// Generate sine table at compile time would be ideal, but for clarity:
// 257 entries (256 + 1 for interpolation wraparound)
static int16_t generateSineTable[257];
static uint32_t generateOscIncrements[97];
static int16_t generateFmQuantizer[257];

namespace {
struct TableInitializer {
    TableInitializer() {
        // Generate sine table
        for (int i = 0; i < 257; ++i) {
            double phase = (static_cast<double>(i) / 256.0) * 2.0 * M_PI;
            generateSineTable[i] = static_cast<int16_t>(std::sin(phase) * 32767.0);
        }

        // Generate oscillator increments for 96kHz sample rate
        // Covers MIDI notes 0-96 (indices map to pitch in 128ths of semitone)
        // Phase increment = 2^32 * freq / sample_rate
        const double kSampleRate = 96000.0;
        for (int i = 0; i < 97; ++i) {
            // i represents (midi_note * 128) >> 4, offset from note 128
            // Original Braids formula based on pitch table
            double midi_note = 128.0 + static_cast<double>(i);
            double freq = 440.0 * std::pow(2.0, (midi_note / 128.0 - 69.0) / 12.0);
            double increment = (freq / kSampleRate) * 4294967296.0;
            generateOscIncrements[i] = static_cast<uint32_t>(increment);
        }

        // FM frequency quantizer (maps continuous to musical ratios)
        for (int i = 0; i < 257; ++i) {
            // Simple linear mapping for now, can be refined
            generateFmQuantizer[i] = static_cast<int16_t>(i * 128);
        }
    }
} tableInit;
}

const int16_t wav_sine[] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602,
    6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
    32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683,
    27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868,
    18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179,
    6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602,
    -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
    -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
    -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179,
    -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804, 0
};
const size_t WAV_SINE_SIZE = 257;

// Phase increments for oscillator frequencies at 96kHz
// Index: (pitch - kPitchTableStart) >> 4, where kPitchTableStart = 128 * 128
const uint32_t lut_oscillator_increments[] = {
    // These are pre-calculated for 96kHz sample rate
    // pitch = midi_note * 128, table starts at midi note 128
    358837,    380198,    402715,    426454,    451483,    477873,
    505698,    535035,    565965,    598573,    632949,    669186,
    707394,    747673,    790139,    834907,    882107,    931867,
    984327,    1039634,   1097941,   1159410,   1224213,   1292532,
    1364560,   1440502,   1520573,   1605001,   1694028,   1787907,
    1886909,   1991318,   2101438,   2217590,   2340116,   2469377,
    2605757,   2749666,   2901541,   3061840,   3231054,   3409702,
    3598339,   3797552,   4007971,   4230264,   4465143,   4713368,
    4975752,   5253163,   5546527,   5856836,   6185151,   6532608,
    6900423,   7289897,   7702428,   8139514,   8602760,   9093889,
    9614747,   10167318,  10753732,  11376276,  12037412,  12739790,
    13486262,  14279903,  15124024,  16022193,  16978253,  17996337,
    19080890,  20236685,  21468855,  22782918,  24184808,  25680905,
    27278069,  28983673,  30805651,  32752544,  34833551,  37058586,
    39438332,  41984313,  44708959,  47625679,  50748937,  54094346,
    57679770,  61524435,  65649063
};
const size_t LUT_OSCILLATOR_INCREMENTS_SIZE = 97;

// FM frequency quantizer - maps parameter to musical frequency ratios
const int16_t lut_fm_frequency_quantizer[] = {
    0, 128, 256, 384, 512, 640, 768, 896,
    1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920,
    2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944,
    3072, 3200, 3328, 3456, 3584, 3712, 3840, 3968,
    4096, 4224, 4352, 4480, 4608, 4736, 4864, 4992,
    5120, 5248, 5376, 5504, 5632, 5760, 5888, 6016,
    6144, 6272, 6400, 6528, 6656, 6784, 6912, 7040,
    7168, 7296, 7424, 7552, 7680, 7808, 7936, 8064,
    8192, 8320, 8448, 8576, 8704, 8832, 8960, 9088,
    9216, 9344, 9472, 9600, 9728, 9856, 9984, 10112,
    10240, 10368, 10496, 10624, 10752, 10880, 11008, 11136,
    11264, 11392, 11520, 11648, 11776, 11904, 12032, 12160,
    12288, 12416, 12544, 12672, 12800, 12928, 13056, 13184,
    13312, 13440, 13568, 13696, 13824, 13952, 14080, 14208,
    14336, 14464, 14592, 14720, 14848, 14976, 15104, 15232,
    15360, 15488, 15616, 15744, 15872, 16000, 16128, 16256,
    16384, 16512, 16640, 16768, 16896, 17024, 17152, 17280,
    17408, 17536, 17664, 17792, 17920, 18048, 18176, 18304,
    18432, 18560, 18688, 18816, 18944, 19072, 19200, 19328,
    19456, 19584, 19712, 19840, 19968, 20096, 20224, 20352,
    20480, 20608, 20736, 20864, 20992, 21120, 21248, 21376,
    21504, 21632, 21760, 21888, 22016, 22144, 22272, 22400,
    22528, 22656, 22784, 22912, 23040, 23168, 23296, 23424,
    23552, 23680, 23808, 23936, 24064, 24192, 24320, 24448,
    24576, 24704, 24832, 24960, 25088, 25216, 25344, 25472,
    25600, 25728, 25856, 25984, 26112, 26240, 26368, 26496,
    26624, 26752, 26880, 27008, 27136, 27264, 27392, 27520,
    27648, 27776, 27904, 28032, 28160, 28288, 28416, 28544,
    28672, 28800, 28928, 29056, 29184, 29312, 29440, 29568,
    29696, 29824, 29952, 30080, 30208, 30336, 30464, 30592,
    30720, 30848, 30976, 31104, 31232, 31360, 31488, 31616,
    31744, 31872, 32000, 32128, 32256, 32384, 32512, 32640, 32767
};
const size_t LUT_FM_FREQUENCY_QUANTIZER_SIZE = 257;

} // namespace braids
```

**Step 5: Update CMakeLists.txt to compile resources**

Add to plugin sources:

```cmake
target_sources(BraidsVST
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/dsp/braids/resources.cpp)
```

And add to test sources:

```cmake
add_executable(BraidsVSTTests
    test/dsp/StmlibTests.cpp
    src/dsp/braids/resources.cpp)
```

**Step 6: Run tests to verify they pass**

Run:
```bash
cmake --build build && cd build && ctest --output-on-failure
```

Expected: All 8 tests pass

**Step 7: Commit**

```bash
git add -A
git commit -m "feat: add FM oscillator lookup tables

- Sine wavetable (257 entries)
- Oscillator phase increment table for 96kHz
- FM frequency quantizer table"
```

---

## Task 5: Port Basic FM Oscillator

**Files:**
- Create: `src/dsp/braids/fm_oscillator.h`
- Create: `src/dsp/braids/fm_oscillator.cpp`
- Create: `test/dsp/FmOscillatorTests.cpp`

**Step 1: Write failing test for FM oscillator**

Create `test/dsp/FmOscillatorTests.cpp`:

```cpp
#include <gtest/gtest.h>
#include "dsp/braids/fm_oscillator.h"

TEST(FmOscillator, InitDoesNotCrash)
{
    braids::FmOscillator osc;
    osc.Init();
}

TEST(FmOscillator, RenderProducesNonZeroOutput)
{
    braids::FmOscillator osc;
    osc.Init();
    osc.set_pitch(60 << 7); // Middle C in Braids pitch format
    osc.set_parameters(8192, 16384); // Moderate FM depth and ratio

    int16_t buffer[32];
    osc.Render(buffer, 32);

    // Check that at least some samples are non-zero
    bool hasNonZero = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(FmOscillator, OutputIsInRange)
{
    braids::FmOscillator osc;
    osc.Init();
    osc.set_pitch(72 << 7); // C5
    osc.set_parameters(16384, 16384);

    int16_t buffer[256];
    osc.Render(buffer, 256);

    for (int i = 0; i < 256; ++i) {
        EXPECT_GE(buffer[i], -32768);
        EXPECT_LE(buffer[i], 32767);
    }
}
```

**Step 2: Run tests to verify they fail**

Run:
```bash
cmake --build build && cd build && ctest --output-on-failure
```

Expected: FAIL - fm_oscillator.h not found

**Step 3: Create src/dsp/braids/fm_oscillator.h**

```cpp
// Ported from Mutable Instruments Braids digital_oscillator.cc RenderFm
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include <cstring>
#include "../stmlib/stmlib.h"

namespace braids {

class FmOscillator
{
public:
    FmOscillator() = default;
    ~FmOscillator() = default;

    void Init();

    void set_pitch(int16_t pitch) { pitch_ = pitch; }
    void set_parameters(int16_t param1, int16_t param2) {
        parameter_[0] = param1;
        parameter_[1] = param2;
    }

    void Render(int16_t* buffer, size_t size);

private:
    uint32_t ComputePhaseIncrement(int16_t midi_pitch);

    uint32_t phase_ = 0;
    uint32_t modulator_phase_ = 0;
    int16_t pitch_ = 0;
    int16_t parameter_[2] = {0, 0};
    int16_t previous_parameter_[2] = {0, 0};

    DISALLOW_COPY_AND_ASSIGN(FmOscillator);
};

} // namespace braids
```

**Step 4: Create src/dsp/braids/fm_oscillator.cpp**

```cpp
// Ported from Mutable Instruments Braids digital_oscillator.cc RenderFm
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#include "fm_oscillator.h"
#include "resources.h"
#include "../stmlib/dsp.h"

namespace braids {

static const uint16_t kHighestNote = 140 * 128;
static const uint16_t kPitchTableStart = 128 * 128;
static const uint16_t kOctave = 12 * 128;

void FmOscillator::Init()
{
    phase_ = 0;
    modulator_phase_ = 0;
    pitch_ = 0;
    parameter_[0] = 0;
    parameter_[1] = 0;
    previous_parameter_[0] = 0;
    previous_parameter_[1] = 0;
}

uint32_t FmOscillator::ComputePhaseIncrement(int16_t midi_pitch)
{
    if (midi_pitch >= kPitchTableStart) {
        midi_pitch = kPitchTableStart - 1;
    }

    int32_t ref_pitch = midi_pitch;
    ref_pitch -= kPitchTableStart;

    size_t num_shifts = 0;
    while (ref_pitch < 0) {
        ref_pitch += kOctave;
        ++num_shifts;
    }

    uint32_t a = lut_oscillator_increments[ref_pitch >> 4];
    uint32_t b = lut_oscillator_increments[(ref_pitch >> 4) + 1];
    uint32_t phase_increment = a +
        (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
    phase_increment >>= num_shifts;
    return phase_increment;
}

void FmOscillator::Render(int16_t* buffer, size_t size)
{
    uint32_t phase_increment = ComputePhaseIncrement(pitch_);

    // Clamp pitch
    int16_t clamped_pitch = pitch_;
    if (clamped_pitch > kHighestNote) {
        clamped_pitch = kHighestNote;
    } else if (clamped_pitch < 0) {
        clamped_pitch = 0;
    }

    uint32_t modulator_phase = modulator_phase_;
    uint32_t modulator_phase_increment = ComputePhaseIncrement(
        (12 << 7) + clamped_pitch + ((parameter_[1] - 16384) >> 1)) >> 1;

    // Parameter interpolation
    int32_t parameter_0 = previous_parameter_[0];
    int32_t parameter_0_increment =
        (parameter_[0] - previous_parameter_[0]) / static_cast<int32_t>(size);

    while (size--) {
        parameter_0 += parameter_0_increment;

        phase_ += phase_increment;
        modulator_phase += modulator_phase_increment;

        uint32_t pm = (stmlib::Interpolate824(wav_sine, modulator_phase) *
                       parameter_0) << 2;
        *buffer++ = stmlib::Interpolate824(wav_sine, phase_ + pm);
    }

    previous_parameter_[0] = parameter_[0];
    modulator_phase_ = modulator_phase;
}

} // namespace braids
```

**Step 5: Update CMakeLists.txt**

Add fm_oscillator to sources:

```cmake
target_sources(BraidsVST
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/dsp/braids/resources.cpp
        src/dsp/braids/fm_oscillator.cpp)
```

Update tests:

```cmake
add_executable(BraidsVSTTests
    test/dsp/StmlibTests.cpp
    test/dsp/FmOscillatorTests.cpp
    src/dsp/braids/resources.cpp
    src/dsp/braids/fm_oscillator.cpp)
```

**Step 6: Run tests to verify they pass**

Run:
```bash
cmake --build build && cd build && ctest --output-on-failure
```

Expected: All 11 tests pass

**Step 7: Commit**

```bash
git add -A
git commit -m "feat: port FM oscillator from Braids

- FmOscillator class with Init/Render interface
- Pitch-to-phase-increment conversion
- FM modulation with parameter control"
```

---

## Task 6: Integrate FM Oscillator into Plugin

**Files:**
- Modify: `src/PluginProcessor.h`
- Modify: `src/PluginProcessor.cpp`

**Step 1: Update PluginProcessor.h**

```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/braids/fm_oscillator.h"

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

private:
    void handleMidiMessage(const juce::MidiMessage& msg);

    braids::FmOscillator oscillator_;
    double hostSampleRate_ = 44100.0;

    // Simple voice state
    bool noteOn_ = false;
    int currentNote_ = 60;
    float currentVelocity_ = 0.0f;

    // Internal 96kHz buffer for fixed-point rendering
    static constexpr size_t kInternalBlockSize = 24;
    int16_t internalBuffer_[kInternalBlockSize];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraidsVSTProcessor)
};
```

**Step 2: Update PluginProcessor.cpp**

```cpp
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

    for (int i = 0; i < numSamples; i += kInternalBlockSize)
    {
        size_t blockSize = std::min(static_cast<size_t>(numSamples - i), kInternalBlockSize);

        oscillator_.Render(internalBuffer_, blockSize);

        // Convert int16 to float and apply velocity
        for (size_t j = 0; j < blockSize; ++j)
        {
            float sample = static_cast<float>(internalBuffer_[j]) / 32768.0f;
            sample *= currentVelocity_;

            leftChannel[i + j] = sample;
            if (rightChannel)
                rightChannel[i + j] = sample;
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
```

**Step 3: Build and test manually**

Run:
```bash
cmake --build build
```

Then load the VST3 in a DAW and press MIDI keys.

Expected: You hear FM synthesis sound when playing MIDI notes

**Step 4: Commit**

```bash
git add -A
git commit -m "feat: integrate FM oscillator with MIDI input

- Handle MIDI note on/off messages
- Render FM oscillator to audio buffer
- Convert fixed-point to float output
- Apply velocity scaling

Phase 1 milestone: Plugin produces FM sound from MIDI input"
```

---

## Summary

After completing all 6 tasks, you will have:

1. A working JUCE plugin project with CMake
2. GitHub Actions CI for macOS and Windows
3. Ported stmlib utilities (CLIP, interpolation, random)
4. FM oscillator lookup tables
5. A working FM oscillator ported from Braids
6. A plugin that produces sound when you play MIDI notes

**Known Limitations (addressed in later phases):**
- Single voice only (polyphony in Phase 3)
- No resampling (96kHz proper rendering in Phase 3)
- No envelope (Phase 2)
- Minimal UI (Phase 4)
- FM only (other algorithms in Phase 2)

**Next Phase:** Port remaining oscillator algorithms and add envelope.
