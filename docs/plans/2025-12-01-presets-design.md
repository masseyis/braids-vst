# Presets System Design for BraidsVST

## Overview

Add a preset system with factory presets and user preset saving, integrated into the tracker-style UI.

## Storage

**Factory presets:** Bundled in plugin Resources folder, read-only. 12 presets covering each oscillator shape.

**User presets:** Saved to `~/Music/BraidsVST/Presets/`. Created on first save. XML format.

**Naming:** Random generation from word lists:
```
Adjectives: Neon, Chrome, Void, Plasma, Quantum, Cyber, Binary, Static, Flux, Dark
Nouns: Grid, Pulse, Signal, Wave, Core, Drone, Circuit, Glitch, Beam, Echo
```
Produces names like "Plasma Circuit.xml". On collision, append number ("Void Echo 2.xml").

## UI

New PRESET row at top (7 rows total, window height 226px):
```
┌────────────────────────────────┐
│         BRAIDS VST             │
├────────────────────────────────┤
│ PRESET   [▓▓▓░░░░░░░] Void Echo│
│ SHAPE    [▓▓▓▓▓▓▓▓░░] FM       │
│ TIMBRE   [▓▓▓▓░░░░░░] 064      │
│ COLOR    [▓▓▓▓▓░░░░░] 064      │
│ ATTACK   [▓░░░░░░░░░] 50ms     │
│ DECAY    [▓▓▓░░░░░░░] 200ms    │
│ VOICES   [▓▓▓▓▓░░░░░] 08       │
└────────────────────────────────┘
```

## Interaction

- **Left/Right on PRESET row** - Cycle through presets
- **Shift+S (anywhere)** - Save new user preset with random name
- **Modified indicator** - Show `*` after name when parameters changed ("Void Echo *")

## Factory Presets

| Name | Shape | Timbre | Color | Attack | Decay | Voices |
|------|-------|--------|-------|--------|-------|--------|
| Init | FM | 64 | 64 | 50ms | 200ms | 8 |
| Neon Lead | Saw | 100 | 40 | 5ms | 300ms | 4 |
| Chrome Bass | Square+Sub | 80 | 90 | 0ms | 400ms | 2 |
| Void Pad | Morph | 60 | 70 | 200ms | 1500ms | 8 |
| Plasma Pluck | FM | 90 | 50 | 0ms | 150ms | 6 |
| Quantum Bell | Sine/Triangle | 110 | 30 | 0ms | 800ms | 8 |
| Cyber Sync | Saw Sync | 70 | 80 | 10ms | 250ms | 4 |
| Binary Buzz | Buzz | 50 | 100 | 5ms | 200ms | 4 |
| Static Drone | Saw+Sub | 40 | 60 | 300ms | 2000ms | 6 |
| Flux Wave | Saw/Square | 85 | 45 | 20ms | 500ms | 4 |
| Dark Core | Square Sync | 30 | 120 | 50ms | 600ms | 2 |
| Glitch Signal | FM | 127 | 127 | 0ms | 100ms | 8 |

## Implementation

**PresetManager class:**
- Load factory presets from bundle Resources
- Scan user folder for .xml files
- Maintain merged sorted preset list
- Save: generate name, write XML, add to list
- Load: read XML, apply to processor parameters

**File format:** Same XML as getStateInformation() with added `<name>` element.
