#pragma once

#include <array>
#include <cstdint>

namespace arp
{

enum class Style : int
{
    Up,
    Down,
    UpDown,
    DownUp,
    UpAndDown,
    DownAndUp,
    Converge,
    Diverge,
    ConAndDiverge,
    PinkyUp,
    PinkyUpDown,
    ThumbUp,
    ThumbUpDown,
    PlayOrder,
    ChordTrigger,
    Random,
    RandomOther,
    RandomOnce,

    // Rhythmic patterns
    Gallop,
    Tresillo,
    OctaveBounce,
    Alberti,
    Stutter,
    Syncopated,
    DottedEighths,
    Pedal,
    Cascade,
    PingPong,

    // Chord rhythms
    OffbeatStabs,
    TresilloChords,
    Charleston,
    Clave,
    Dembow,
    PulseAccents,
    TranceGate,
    PianoComp,
    Ballad,
    Skank,
    GallopChords,
    StutterChords
};

constexpr int numStyles = 40;

enum class Groove : int { Straight, Swing8, Swing16 };
enum class Retrigger : int { Off, Note, Beat };
enum class TransposeMode : int { Shift, Key };

struct Params
{
    Style style = Style::Up;

    bool sync = true;
    double syncRateBeats = 0.25;   // step length in quarter notes
    double freeRateMs = 125.0;     // step length when sync is off
    double gate = 0.5;             // fraction of a step (0.01 .. 2.0)

    Groove groove = Groove::Straight;
    double swing = 0.6;            // 0.5 = straight .. 0.75 = hard swing

    bool hold = false;
    int offset = 0;                // rotates the pattern start
    int repeats = 0;               // 0 = infinite

    Retrigger retrigger = Retrigger::Off;
    double retriggerBeats = 4.0;   // used in Beat mode

    TransposeMode transposeMode = TransposeMode::Shift;
    int transposeKey = 0;          // 0 = C .. 11 = B
    int transposeScale = 0;        // index into scales()
    int transposeDistance = 12;    // semitones (Shift) or scale degrees (Key)
    int transposeSteps = 0;        // number of transposed repetitions

    bool velocityOn = false;
    double velocityDecayMs = 1000.0;
    int velocityTarget = 0;
    bool velocityRetrigger = true;
};

struct Transport
{
    double bpm = 120.0;
    bool playing = false;
    bool hasPosition = false;
    double ppqPosition = 0.0;      // host position at the start of the block
};

struct NoteEvent
{
    int sampleOffset = 0;
    bool isNoteOn = true;
    int channel = 1;
    int note = 60;
    int velocity = 100;
};

/** A copy of what the arpeggiator is doing, for the editor to draw. */
struct Snapshot
{
    static constexpr int maxSteps = 64;
    static constexpr int maxNotesPerStep = 8;

    struct Step
    {
        uint8_t count = 0;    // 0 = rest
        uint8_t length = 1;   // in steps, longer for tied notes
        std::array<uint8_t, maxNotesPerStep> notes {};
    };

    std::array<Step, maxSteps> steps {};
    int numSteps = 0;
    int currentStep = -1;
    int transposeIndex = 0;
    int transposeSteps = 0;
    uint32_t stepCounter = 0;
    bool active = false;
};

} // namespace arp
