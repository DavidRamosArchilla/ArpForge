# ArpForge

A MIDI arpeggiator plugin (VST3) for FL Studio, modelled on Ableton Live's Arpeggiator.
It never changes the notes you write: it reads the notes being played and generates the
arpeggio from them in real time.

## Using it in FL Studio

FL Studio's mixer only carries audio, so ArpForge goes in front of the synth, not in the
mixer. It sends its notes to the synth through an FL Studio MIDI port:

1. Add **ArpForge** to the Channel Rack (it appears among the generators after a plugin
   scan: *Options → Manage plugins → Find installed plugins*).
2. Open ArpForge, click the gear icon (plugin wrapper settings) and set
   **MIDI → Output port** to a free number, e.g. `1`.
3. Add your synth to the Channel Rack, open its wrapper settings and set
   **MIDI → Input port** to the same number.
4. Write or play your chords on the **ArpForge** channel. The synth plays the arpeggio.

Each ArpForge/synth pair needs its own port number.

## Controls

| Section   | Control                | What it does |
|-----------|------------------------|--------------|
| Pattern   | Style                  | Up, Down, UpDown, DownUp, Up & Down, Down & Up, Converge, Diverge, Con & Diverge, Pinky Up, Pinky UpDown, Thumb Up, Thumb UpDown, Play Order, Chord Trigger, Random, Random Other, Random Once |
| Rhythm    | Sync / Rate            | Step length synced to the tempo (1/1 … 1/64, dotted and triplets) or free in ms |
|           | Gate                   | Note length as a percentage of a step (1–200%) |
|           | Groove / Swing         | Straight, Swing 8 or Swing 16, with swing amount |
| Header    | Hold                   | Keeps arpeggiating after the keys are released; new notes pressed after releasing replace the held ones |
| Sequence  | Retrigger              | Off, on every new Note, or every Beat interval (Every) |
|           | Offset                 | Starts the pattern N steps later |
|           | Repeats                | Plays the pattern N times then stops (∞ = forever) |
| Transpose | Shift / Key            | Transpose in semitones, or in degrees of the chosen key and scale |
|           | Distance / Steps       | How far each pass moves, and how many transposed passes follow the original |
| Velocity  | On / Decay / Target    | Moves velocity from the played velocity to Target over Decay time |
|           | Retrig                 | Restarts the velocity ramp when the pattern retriggers |

The rate and Retrigger "Beat" intervals follow the song position while the transport plays,
so steps stay on the grid, including in loops.

## Building

Requirements on Windows: Visual Studio 2022 Build Tools (C++ workload) and CMake.
JUCE is a git submodule:

```sh
git submodule update --init
scripts/build-windows.sh            # from WSL: build, test, install into Common Files\VST3
```

Engine unit tests run on Linux too:

```sh
make -C tests test
```

## Layout

- `src/engine/` — the arpeggiator itself, plain C++ with no JUCE dependency
- `src/PluginProcessor.*` — JUCE plugin, parameters, MIDI in/out
- `src/PluginEditor.*`, `src/ui/` — the interface
- `tests/` — engine tests
- `resources/fonts/` — Inter (SIL Open Font License)
