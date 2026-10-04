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

### Instruments without a MIDI input port (Sytrus, FLEX, 3x Osc...)

FL Studio's native plugins can't receive notes from a MIDI port. Use **capture & drag**
instead: ArpForge records everything it plays, and the **DRAG MIDI** tile in the Pattern
panel turns it into a MIDI clip.

1. Play your chords through ArpForge (with or without a synth connected).
2. Drag the **DRAG MIDI** tile into the instrument's piano roll or the Playlist. Click it
   instead to save a `.mid` file.

A take runs from pressing play to stopping, and it starts on a bar line so the clip drops in
time. With the transport stopped, a take is one live performance; a 2-second pause starts a
new one. The orange dot means it's capturing.

## Controls

| Section   | Control                | What it does |
|-----------|------------------------|--------------|
| Pattern   | Style                  | **Classic:** Up, Down, UpDown, DownUp, Up & Down, Down & Up, Converge, Diverge, Con & Diverge, Pinky Up, Pinky UpDown, Thumb Up, Thumb UpDown, Play Order, Chord Trigger, Random, Random Other, Random Once. **Rhythmic:** Gallop, Tresillo, Octave Bounce, Alberti, Stutter, Syncopated, Dotted 8ths, Pedal, Cascade, Ping Pong. **Chord rhythms:** Offbeat Stabs, Tresillo Chords, Charleston, Clave 3-2, Dembow, Pulse Accents, Trance Gate, Piano Comp, Ballad, Skank, Gallop Chords, Stutter Chords |
| Rhythm    | Sync / Rate            | Step length synced to the tempo (1/1 … 1/64, dotted and triplets) or free in ms |
|           | Gate                   | Note length as a percentage of a step (1–200%) |
|           | Groove / Swing         | Straight, Swing 8 or Swing 16, with swing amount |
| Header    | Presets                | 27 factory presets: classic arps, rhythmic patterns and chord rhythms |
| Pattern   | Drag MIDI              | Drag the captured arpeggio into a piano roll, or click to save it as a `.mid` file |
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

## Installing a release

Download the zip from [Releases](https://github.com/DavidRamosArchilla/ArpForge/releases),
unzip it and copy `ArpForge.vst3` to `C:\Program Files\Common Files\VST3`. The plugin isn't
code-signed: if Windows blocks it, right-click the zip → Properties → Unblock before unzipping.

## Building

Requirements on Windows: Visual Studio 2022 Build Tools (C++ workload) and CMake.
JUCE is a git submodule:

```sh
git submodule update --init
scripts/build-windows.sh            # from WSL: build, test, install into Common Files\VST3
```

Releases are built by GitHub Actions: pushing a tag like `v0.2.0` builds, tests and publishes
`ArpForge-0.2.0-win64.zip` (see `.github/workflows/release.yml`).

Engine unit tests run on Linux too:

```sh
make -C tests test
```

## Layout

- `src/engine/` — the arpeggiator itself, plain C++ with no JUCE dependency
- `src/capture/` — capture of generated notes and MIDI file export
- `src/PluginProcessor.*` — JUCE plugin, parameters, MIDI in/out
- `src/PluginEditor.*`, `src/ui/` — the interface
- `tests/` — engine and capture tests
- `tools/` — dev tools: `UiSnapshot` renders the UI to PNG, `HostTest` loads the VST3 like a DAW
- `resources/fonts/` — Inter (SIL Open Font License)

## License

ArpForge is free software under the [GNU Affero General Public License v3](LICENSE).
It is built with [JUCE](https://juce.com) (AGPLv3) and the VST3 SDK (MIT), and uses the
Inter typeface (SIL Open Font License).
