# Tracker-Style UI Design for BraidsVST

## Overview

Replace the current knob-based UI with an M8-style tracker interface featuring:
- Two-column grid layout (label + value)
- Horizontal value bars behind text
- Full keyboard navigation
- Mouse drag and scroll support

## Layout

**Window:** 320x200 pixels

```
┌────────────────────────────────┐
│         BRAIDS VST             │
├────────────────────────────────┤
│ SHAPE    [▓▓▓▓▓▓▓▓░░] FM       │
│ TIMBRE   [▓▓▓▓░░░░░░] 064      │
│ COLOR    [▓▓▓▓▓░░░░░] 064      │
│ ATTACK   [▓░░░░░░░░░] 50ms     │
│ DECAY    [▓▓▓░░░░░░░] 200ms    │
│ VOICES   [▓▓▓▓▓░░░░░] 08       │
└────────────────────────────────┘
```

## Colors

| Element | Color |
|---------|-------|
| Background | #1a1a1a |
| Row background (unfilled) | #0d0d0d |
| Value bar fill | #4a9090 (muted cyan) |
| Selected row bar | #6abfbf (brighter cyan) |
| Selected row text | #ffffff |
| Unselected text | #a0a0a0 |

**Font:** Monospace, ~14px

## Keyboard Navigation

### Movement
- **Up/Down arrows** - Move selection between rows
- **Wraps around** - Down from Voices → Shape, Up from Shape → Voices

### Value Adjustment
- **Left/Right arrows** - Decrease/increase by 1 step
- **Shift+Left/Right** - Decrease/increase by 10% of range

## Parameters

| Parameter | Range | Display | Small Step | Large Step (10%) |
|-----------|-------|---------|------------|------------------|
| Shape | 10 types | Name | 1 | 1 |
| Timbre | 0-127 | "000"-"127" | 1 | 13 |
| Color | 0-127 | "000"-"127" | 1 | 13 |
| Attack | 0-500ms | "0ms"-"500ms" | 5ms | 50ms |
| Decay | 10-2000ms | "10ms"-"2000ms" | 20ms | 200ms |
| Voices | 1-16 | "01"-"16" | 1 | 2 |

## Mouse Support

- **Click row** - Select that row
- **Drag left/right on row** - Adjust value
- **Scroll wheel on row** - Fine adjustment

## Processor Mapping

| Parameter | Processor Conversion |
|-----------|---------------------|
| Shape | Direct index to AudioParameterChoice |
| Timbre | value/127.0f → AudioParameterFloat |
| Color | value/127.0f → AudioParameterFloat |
| Attack | ms/500.0f → AudioParameterFloat |
| Decay | (ms-10)/1990.0f → AudioParameterFloat |
| Voices | Direct to AudioParameterInt |

## Component Structure

- `TrackerRow` - Reusable: label, value bar, value text, mouse drag handling
- `TrackerEditor` - Main editor: 6 rows, keyboard focus, selection state

## Implementation Notes

- Editor grabs keyboard focus on click
- `keyPressed()` handles all navigation centrally
- No child component focus - editor manages everything
- Value bar: filled rect width = (value/max) × row width
