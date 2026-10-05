#pragma once

#include "ArpTypes.h"
#include "Patterns.h"
#include <cstdint>
#include <random>
#include <vector>

namespace arp
{

/** Real-time arpeggiator. Takes the notes being played, never changes them,
    and generates a new note stream from them. All timing is kept in beats on
    an internal clock that is aligned to the host's song position whenever the
    transport is running. */
class Engine
{
public:
    Engine();

    void prepare (double sampleRate);
    void setParams (const Params& newParams);
    const Params& getParams() const { return params; }

    /** Forgets everything without emitting note-offs. */
    void reset();

    /** Processes one block. `input` must be sorted by sample offset; generated
        events are appended to `output` in time order. */
    void process (const Transport& transport, int numSamples,
                  const std::vector<NoteEvent>& input, std::vector<NoteEvent>& output);

    /** Releases every sounding note and forgets held notes (MIDI panic). */
    void releaseAll (int sampleOffset, std::vector<NoteEvent>& output);

    void fillSnapshot (Snapshot& snapshot);

private:
    struct HeldNote
    {
        int note, channel, velocity;
        uint64_t order;
    };

    struct SoundingNote
    {
        int note, channel;
        double offBeat;
    };

    // Timing helpers (beats on the internal clock).
    double beatAt (int sample) const     { return blockStartBeat + sample * beatsPerSample; }
    double secondsAt (int sample) const  { return blockStartSeconds + sample / sampleRate; }
    double hostPosAt (int sample) const  { return beatAt (sample) + gridOffset; }
    int sampleFor (double beat, int notBefore, int numSamples) const;
    double stepLength() const;
    double warp (double hostPos) const;
    double unwarp (double hostPos) const;
    int64_t firstGridIndexAtOrAfter (double hostPos) const;
    void scheduleGridStep (int64_t index);

    void handleNote (const NoteEvent& e);
    void afterEventGroup (int sample);
    void start (int sample);
    void resetPattern (int sample);
    void fireStep (int sample, std::vector<NoteEvent>& out);
    void emitNoteOffsUpTo (int sample, int numSamples, std::vector<NoteEvent>& out);
    void rebuildSequence();
    void shuffleSequence();
    void syncActiveWithHold();
    int transposeNote (int note) const;

    /** Calls fn (heldNote, semitoneShift) for each note a step plays. */
    template <typename Fn>
    void forEachNoteOf (const Step& step, Fn&& fn) const;
    int velocityFor (int noteVelocity, int sample) const;

    Params params;
    double sampleRate = 44100.0;

    // Clock
    double engineBeat = 0.0, engineSeconds = 0.0;
    double blockStartBeat = 0.0, blockStartSeconds = 0.0;
    double beatsPerSample = 0.0;
    double gridOffset = 0.0;          // host position = engine beat + gridOffset
    bool hostGrid = false;
    double nextStepNotBefore = 0.0;   // host position the next grid step must not precede
    int blocksSinceJump = 0;
    int blocksOffGrid = 0;

    // Notes
    std::vector<HeldNote> physical;   // keys currently down
    std::vector<HeldNote> active;     // notes being arpeggiated (keys + latched)
    std::vector<HeldNote> sorted;     // active, by pitch
    std::vector<Step> sequence;
    std::vector<SoundingNote> sounding;
    mutable std::vector<std::pair<int, int>> stepNotes;   // scratch: (sorted index, shift)
    uint64_t orderCounter = 0;
    bool sequenceDirty = true;

    // Pattern state
    bool running = false;
    bool finished = false;
    bool immediateStep = false;
    bool groupHadNewNote = false;
    bool groupStartedEmpty = false;
    double nextStepBeat = 0.0;
    int64_t nextGridIndex = 0;
    int position = 0;
    int transposeIndex = 0;
    int cycles = 0;
    int64_t lastRetriggerBlock = 0;
    double velocityRampStart = 0.0;

    // Display
    int lastDisplayStep = -1;
    uint32_t stepCounter = 0;

    std::mt19937 rng;
};

} // namespace arp
