# Phase 2: DSP Completion Implementation Plan

**Goal:** Port remaining oscillator algorithms, add envelope, and expose shape selection.

**Approach:** Rather than port all 47 algorithms at once, we'll:
1. Create the MacroOscillator architecture that dispatches to algorithms
2. Port analog oscillators (classic waveforms)
3. Port a representative set of digital algorithms
4. Add envelope
5. Expose shape as JUCE parameter

---

## Task 1: Create MacroOscillator Architecture

**Files:**
- Create: `src/dsp/braids/macro_oscillator.h`
- Create: `src/dsp/braids/macro_oscillator.cpp`
- Create: `test/dsp/MacroOscillatorTests.cpp`
- Modify: `CMakeLists.txt`

**Overview:**
MacroOscillator is the main entry point that wraps all oscillator types. It has a `set_shape()` method and dispatches `Render()` to the appropriate algorithm.

**Shape enum (subset for Phase 2):**
```cpp
enum MacroOscillatorShape {
    MACRO_OSC_SHAPE_CSAW,           // 0 - Classic saw
    MACRO_OSC_SHAPE_MORPH,          // 1 - Morphing waveforms
    MACRO_OSC_SHAPE_SAW_SQUARE,     // 2 - Saw to square
    MACRO_OSC_SHAPE_SINE_TRIANGLE,  // 3 - Sine to triangle
    MACRO_OSC_SHAPE_BUZZ,           // 4 - Buzz
    MACRO_OSC_SHAPE_SQUARE_SUB,     // 5 - Square with sub
    MACRO_OSC_SHAPE_SAW_SUB,        // 6 - Saw with sub
    MACRO_OSC_SHAPE_SQUARE_SYNC,    // 7 - Hard sync square
    MACRO_OSC_SHAPE_SAW_SYNC,       // 8 - Hard sync saw
    MACRO_OSC_SHAPE_FM,             // 9 - FM (already done)
    MACRO_OSC_SHAPE_LAST
};
```

---

## Task 2: Port Analog Oscillator

**Files:**
- Create: `src/dsp/braids/analog_oscillator.h`
- Create: `src/dsp/braids/analog_oscillator.cpp`
- Create: `test/dsp/AnalogOscillatorTests.cpp`
- Modify: `src/dsp/braids/resources.h` (add bandlimited wavetables)
- Modify: `src/dsp/braids/resources.cpp`

**Algorithms to port:**
- RenderSaw (bandlimited sawtooth)
- RenderSquare (bandlimited square with PWM)
- RenderTriangle
- RenderSine

---

## Task 3: Port AD Envelope

**Files:**
- Create: `src/dsp/braids/envelope.h`
- Create: `test/dsp/EnvelopeTests.cpp`

**From original Braids envelope.h:**
- Simple AD envelope
- Attack/Decay times from lookup table
- Velocity scaling

---

## Task 4: Integrate into MacroOscillator

**Files:**
- Modify: `src/dsp/braids/macro_oscillator.cpp`
- Modify: `test/dsp/MacroOscillatorTests.cpp`

Wire up AnalogOscillator and FmOscillator to MacroOscillator dispatch.

---

## Task 5: Add Shape Parameter to Plugin

**Files:**
- Modify: `src/PluginProcessor.h`
- Modify: `src/PluginProcessor.cpp`

Add JUCE AudioParameterChoice for shape selection.

---

## Task 6: Add Envelope to Plugin

**Files:**
- Modify: `src/PluginProcessor.h`
- Modify: `src/PluginProcessor.cpp`

Integrate envelope with velocity from MIDI.

---

## Detailed Implementation

### Task 1: MacroOscillator Architecture

**Step 1: Create test file `test/dsp/MacroOscillatorTests.cpp`:**

```cpp
#include <gtest/gtest.h>
#include "dsp/braids/macro_oscillator.h"

TEST(MacroOscillator, InitDoesNotCrash)
{
    braids::MacroOscillator osc;
    osc.Init();
}

TEST(MacroOscillator, SetShapeWorks)
{
    braids::MacroOscillator osc;
    osc.Init();
    osc.set_shape(braids::MACRO_OSC_SHAPE_FM);
    // Should not crash
}

TEST(MacroOscillator, RenderFmProducesOutput)
{
    braids::MacroOscillator osc;
    osc.Init();
    osc.set_shape(braids::MACRO_OSC_SHAPE_FM);
    osc.set_pitch(60 << 7);
    osc.set_parameters(8192, 16384);

    int16_t buffer[32];
    uint8_t sync[32] = {0};
    osc.Render(sync, buffer, 32);

    bool hasNonZero = false;
    for (int i = 0; i < 32; ++i) {
        if (buffer[i] != 0) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}
```

**Step 2: Create `src/dsp/braids/macro_oscillator.h`:**

```cpp
#pragma once

#include <cstdint>
#include "../stmlib/stmlib.h"
#include "analog_oscillator.h"
#include "fm_oscillator.h"

namespace braids {

enum MacroOscillatorShape {
    MACRO_OSC_SHAPE_CSAW,
    MACRO_OSC_SHAPE_MORPH,
    MACRO_OSC_SHAPE_SAW_SQUARE,
    MACRO_OSC_SHAPE_SINE_TRIANGLE,
    MACRO_OSC_SHAPE_BUZZ,
    MACRO_OSC_SHAPE_SQUARE_SUB,
    MACRO_OSC_SHAPE_SAW_SUB,
    MACRO_OSC_SHAPE_SQUARE_SYNC,
    MACRO_OSC_SHAPE_SAW_SYNC,
    MACRO_OSC_SHAPE_FM,
    MACRO_OSC_SHAPE_LAST
};

class MacroOscillator {
public:
    MacroOscillator() = default;
    ~MacroOscillator() = default;

    void Init();
    void set_shape(MacroOscillatorShape shape) { shape_ = shape; }
    void set_pitch(int16_t pitch) { pitch_ = pitch; }
    void set_parameters(int16_t p1, int16_t p2) {
        parameter_[0] = p1;
        parameter_[1] = p2;
    }

    void Render(const uint8_t* sync, int16_t* buffer, size_t size);

private:
    MacroOscillatorShape shape_ = MACRO_OSC_SHAPE_FM;
    int16_t pitch_ = 0;
    int16_t parameter_[2] = {0, 0};

    AnalogOscillator analog_oscillator_;
    FmOscillator fm_oscillator_;

    DISALLOW_COPY_AND_ASSIGN(MacroOscillator);
};

} // namespace braids
```

**Step 3: Create `src/dsp/braids/macro_oscillator.cpp`:**

```cpp
#include "macro_oscillator.h"
#include "resources.h"

namespace braids {

void MacroOscillator::Init()
{
    analog_oscillator_.Init();
    fm_oscillator_.Init();
    shape_ = MACRO_OSC_SHAPE_FM;
    pitch_ = 0;
    parameter_[0] = 0;
    parameter_[1] = 0;
}

void MacroOscillator::Render(const uint8_t* sync, int16_t* buffer, size_t size)
{
    // Configure sub-oscillators
    analog_oscillator_.set_pitch(pitch_);
    analog_oscillator_.set_parameter(parameter_[0]);
    fm_oscillator_.set_pitch(pitch_);
    fm_oscillator_.set_parameters(parameter_[0], parameter_[1]);

    switch (shape_) {
        case MACRO_OSC_SHAPE_CSAW:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SAW_SQUARE:
        case MACRO_OSC_SHAPE_MORPH:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SINE_TRIANGLE:
            analog_oscillator_.set_shape(OSC_SHAPE_TRIANGLE);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SQUARE_SUB:
        case MACRO_OSC_SHAPE_BUZZ:
            analog_oscillator_.set_shape(OSC_SHAPE_SQUARE);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SAW_SUB:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SQUARE_SYNC:
            analog_oscillator_.set_shape(OSC_SHAPE_SQUARE);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_SAW_SYNC:
            analog_oscillator_.set_shape(OSC_SHAPE_SAW);
            analog_oscillator_.Render(sync, buffer, size);
            break;

        case MACRO_OSC_SHAPE_FM:
        default:
            fm_oscillator_.Render(buffer, size);
            break;
    }
}

} // namespace braids
```

---

### Task 2: Analog Oscillator (abbreviated - full code in implementation)

Core algorithms:
- Bandlimited saw using polynomial approximation
- Square with PWM from parameter
- Triangle and sine from wavetable

---

### Task 3: AD Envelope

```cpp
// src/dsp/braids/envelope.h
#pragma once
#include <cstdint>

namespace braids {

enum EnvelopeSegment {
    ENV_SEGMENT_ATTACK,
    ENV_SEGMENT_DECAY,
    ENV_SEGMENT_DEAD
};

class Envelope {
public:
    void Init();
    void Trigger(uint16_t attack, uint16_t decay);
    uint16_t Render();  // Returns 0-65535
    bool done() const { return segment_ == ENV_SEGMENT_DEAD; }

private:
    EnvelopeSegment segment_ = ENV_SEGMENT_DEAD;
    uint32_t phase_ = 0;
    uint32_t phase_increment_ = 0;
    uint16_t attack_ = 0;
    uint16_t decay_ = 0;
};

} // namespace braids
```

---

### Task 5-6: Plugin Integration

Add to PluginProcessor:
- `juce::AudioParameterChoice* shapeParam`
- `braids::Envelope envelope_`
- Attack/Decay parameters

---

## Verification

After each task:
1. `cmake --build build`
2. `cd build && ctest --output-on-failure`

After Phase 2:
- Load plugin in DAW
- Change shape parameter - should hear different waveforms
- Envelope should shape amplitude on note-on
