# BraidsVST Design Document

**Date:** 2025-12-01
**Status:** Approved

## Overview

BraidsVST is a polyphonic VST3/AU plugin porting the Mutable Instruments Braids digital oscillator to desktop DAWs. It features keyboard-first navigation, configurable polyphony, and preserves the original Braids sound through fixed-point DSP.

## Key Decisions

| Aspect | Decision |
|--------|----------|
| Platforms | macOS + Windows (Linux later) |
| Polyphony | 1-16 voices, user-configurable |
| DSP approach | Fixed-point core, float at boundaries |
| Sample rate | 96kHz internal, resample to host |
| UI model | Keyboard-first, mouse-optional |
| Visual style | Functional minimal |
| Navigation | Modal focus (shape selector primary) |
| MIDI | Standard (note on/off, bend, mod wheel, velocity) |
| Envelope | Internal AD exposed |
| Parameters | 0-127 display, smoothed internally |
| Formats | VST3 + AU + Standalone |
| Presets | Factory presets v1.0, full browser later |
| License | GPL v3 |
| Name | BraidsVST |

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                    JUCE Plugin                       │
│  ┌───────────────┐  ┌────────────────────────────┐  │
│  │   UI Layer    │  │      Audio Processor       │  │
│  │  (Keyboard    │  │  ┌──────────────────────┐  │  │
│  │   Navigation) │  │  │   Voice Manager      │  │  │
│  │               │◄─┼─►│   (1-16 voices)      │  │  │
│  │  Modal Focus  │  │  │  ┌────────────────┐  │  │  │
│  │  Shape Select │  │  │  │ BraidsVoice    │  │  │  │
│  │  Param Pages  │  │  │  │ (fixed-point)  │  │  │  │
│  └───────────────┘  │  │  └────────────────┘  │  │  │
│                     │  └──────────────────────┘  │  │
│                     │  ┌──────────────────────┐  │  │
│                     │  │ Resampler (96k→host)│  │  │
│                     │  └──────────────────────┘  │  │
│                     └────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### Key Boundaries

- **Fixed-point core**: Braids DSP stays as int16/int32, preserving original sound
- **Float conversion**: Only at audio buffer boundaries
- **96kHz internal**: DSP runs at Braids' native rate; quality resampler converts to host sample rate

## DSP Porting Strategy

### Source Files to Port

| File | Purpose |
|------|---------|
| `macro_oscillator.cc/h` | Entry point, dispatches to render methods |
| `analog_oscillator.cc/h` | Classic waveforms (saw, square, etc.) |
| `digital_oscillator.cc/h` | 34 digital algorithms |
| `resources.cc/h` | Lookup tables, wavetables (~389KB) |
| `envelope.h` | AD envelope |
| `svf.h` | State variable filter |
| `excitation.h` | Excitation source for physical models |
| `quantizer.cc/h` | Pitch quantizer |
| `signature_waveshaper.h` | Waveshaping |

### Porting Transformations

1. **ARM intrinsics** → Portable C++
   - `__SMLAL` → standard multiply + add
   - `__SSAT` → `std::clamp` or inline function

2. **stmlib dependencies** → Extract needed utilities
   - `DISALLOW_COPY_AND_ASSIGN` → `= delete`
   - Fixed-point math helpers → header-only utilities

3. **Namespace** → Wrap in `braidsvst::dsp`

## Voice Management

```cpp
class BraidsVoice {
    MacroOscillator oscillator_;  // Fixed-point DSP core
    Envelope envelope_;            // AD envelope

    int16_t pitch_;               // Current pitch (MIDI note + bend)
    int16_t timbre_;              // Parameter 1
    int16_t color_;               // Parameter 2

    float velocity_;              // Note velocity (scales envelope)
    VoiceState state_;            // FREE, ACTIVE, RELEASING
    uint32_t note_id_;            // For voice stealing decisions
};
```

- **Allocation strategy**: Round-robin with oldest-note stealing
- **Pitch bend**: Applied globally (±2 semitones default)
- **Mod wheel**: Mapped to Timbre by default

## UI Navigation

### Layout

```
┌──────────────────────────────────────────────────────┐
│  BraidsVST                                    [v1.0] │
├──────────────────────────────────────────────────────┤
│  SHAPE                                               │
│  ┌────────────────────────────────────────────────┐  │
│  │  ► FM                                          │  │
│  └────────────────────────────────────────────────┘  │
│                                                      │
│  TIMBRE          COLOR           PITCH              │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐       │
│  │   64     │    │   32     │    │   +0     │       │
│  └──────────┘    └──────────┘    └──────────┘       │
│                                                      │
│  ATTACK          DECAY           VOICES             │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐       │
│  │   12     │    │   48     │    │    8     │       │
│  └──────────┘    └──────────┘    └──────────┘       │
└──────────────────────────────────────────────────────┘
```

### Keyboard Controls

| Key | Action |
|-----|--------|
| `↑` / `↓` | Change shape (in shape selector) |
| `Enter` | Enter parameter page |
| `Escape` | Return to shape selector |
| `Tab` / `Shift+Tab` | Cycle parameters |
| `←` / `→` | Adjust value |
| `Shift + ←/→` | Fine adjust |

## Project Structure

```
braids-vst/
├── CMakeLists.txt
├── LICENSE                     # GPL v3
├── README.md
├── JUCE/                       # Submodule
├── src/
│   ├── PluginProcessor.cpp/h
│   ├── PluginEditor.cpp/h
│   ├── dsp/
│   │   ├── braids/             # Ported DSP
│   │   ├── stmlib/             # Ported utilities
│   │   ├── BraidsVoice.cpp/h
│   │   ├── VoiceManager.cpp/h
│   │   └── Resampler.cpp/h
│   └── ui/
│       ├── LookAndFeel.cpp/h
│       ├── ShapeSelector.cpp/h
│       ├── ParameterPanel.cpp/h
│       └── KeyboardNavigator.cpp/h
├── test/
│   ├── dsp/
│   ├── ui/
│   └── reference/
└── presets/factory/
```

## Testing Strategy

### Test Layers

1. **Unit Tests (DSP Core)**: Per-algorithm reference tests, envelope, fixed-point math
2. **Component Tests**: VoiceManager, Resampler, parameter smoothing
3. **Integration Tests**: Full plugin render, MIDI → audio validation

### Reference Testing

Generate output from original Braids code, compare ported implementation. Bit-exact where possible, tolerance at float boundaries.

### Framework

- GoogleTest
- GitHub Actions CI (macOS + Windows)

## Implementation Phases

### Phase 1 - Foundation + First Sound
- Project setup (CMake, JUCE, GoogleTest)
- GitHub Actions CI pipeline
- Port `stmlib` utilities
- Port `resources.cc`
- Port one algorithm (FM)
- Basic `PluginProcessor` with single voice
- Basic MIDI note on/off
- **Milestone: Load in DAW, play notes, hear FM sound**

### Phase 2 - DSP Completion
- Port remaining oscillators (systematic, tested)
- Port envelope with velocity
- Shape selection via parameter

### Phase 3 - Polyphony
- `VoiceManager` with allocation/stealing
- Resampler (96kHz → host)
- Full MIDI (pitch bend, mod wheel)

### Phase 4 - UI
- `LookAndFeel` (minimal dark theme)
- Shape selector with keyboard navigation
- Parameter panel
- Full keyboard navigation

### Phase 5 - Polish
- Factory presets
- Parameter smoothing tuning
- Performance optimization
- Build AU + VST3 + Standalone

## Licensing

- **Original Braids DSP**: Copyright 2012 Emilie Gillet (MIT License)
- **BraidsVST plugin**: GPL v3
- **JUCE**: Used under GPL v3

## Credits

- DSP algorithms ported from [Mutable Instruments Braids](https://github.com/pichenettes/eurorack) by Emilie Gillet
- Built with [JUCE](https://juce.com)
