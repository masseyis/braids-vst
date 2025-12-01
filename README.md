# BraidsVST

A polyphonic VST3/AU plugin porting the [Mutable Instruments Braids](https://mutable-instruments.net/modules/braids/) macro-oscillator to your DAW.

![BraidsVST Screenshot](Braids-screenshot.png)

## Features

- **10 oscillator shapes** from the original Braids: CSAW, Morph, Saw Square, Sine Triangle, Buzz, Square Sub, Saw Comb, Reso Triangle, Reso Saw, and Fold
- **Up to 16 voices** of polyphony
- **Tracker-style UI** inspired by the Dirtywave M8
- **12 factory presets** with sci-fi themed names
- **User preset saving** to external folder

## Installation

### Download

Grab the latest release from the [Releases page](https://github.com/masseyis/braids-vst/releases).

- **macOS**: VST3, AU, and Standalone app
- **Windows**: VST3 and Standalone
- **Linux**: VST3 and Standalone

### Plugin Locations

| Platform | VST3 Location |
|----------|---------------|
| macOS | `~/Library/Audio/Plug-Ins/VST3/` |
| Windows | `C:\Program Files\Common Files\VST3\` |
| Linux | `~/.vst3/` |

## Usage

### Keyboard Controls

| Key | Action |
|-----|--------|
| Up/Down | Navigate between rows |
| Left/Right | Adjust value |
| Shift + Left/Right | Adjust value by 10% |
| Shift + S | Save current settings as new preset |

### Mouse Controls

- **Click** a row to select it
- **Drag** left/right to adjust values
- **Scroll wheel** to adjust values

### Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Preset | - | Browse factory and user presets |
| Shape | 0-9 | Oscillator waveform shape |
| Timbre | 0-127 | Primary tonal character control |
| Color | 0-127 | Secondary tonal modifier |
| Attack | 0-500ms | Amplitude envelope attack time |
| Decay | 10-2000ms | Amplitude envelope decay time |
| Voices | 1-16 | Maximum polyphony |

### User Presets

User presets are saved to:
- **macOS/Linux**: `~/Music/BraidsVST/Presets/`
- **Windows**: `%USERPROFILE%\Music\BraidsVST\Presets\`

## Building from Source

### Requirements

- CMake 3.22+
- C++17 compiler
- **Linux only**: ALSA, JACK, and X11 development libraries

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Run Tests

```bash
ctest --test-dir build --build-config Release
```

### Build Artifacts

After building, find the plugins in:
- `build/BraidsVST_artefacts/Release/VST3/`
- `build/BraidsVST_artefacts/Release/AU/` (macOS only)
- `build/BraidsVST_artefacts/Release/Standalone/`

## Credits

- Original Braids DSP code by [Mutable Instruments](https://mutable-instruments.net/) (MIT License)
- Built with [JUCE](https://juce.com/)

## Support

If you find this plugin useful, consider supporting development:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/masseyis)

## License

MIT License - See [LICENSE](LICENSE) for details.
