#pragma once

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace capture
{

/** Collects the notes ArpForge generates into a "take" that can be exported
    as a MIDI clip.

    A take starts when the transport starts, or, with the transport stopped,
    when notes start after a pause. While the transport plays, the take is
    bar-aligned on the song grid, so a dropped clip lines up with the song. */
class CaptureTake
{
public:
    enum class Kind : uint8_t { NoteOn, NoteOff, TransportStart, TransportStop };

    struct Event
    {
        Kind kind = Kind::NoteOn;
        double beat = 0.0;       // song position while playing, a free-running beat clock otherwise
        double seconds = 0.0;    // wall-clock-ish time, for detecting pauses
        bool playing = false;
        int channel = 1, note = 60, velocity = 100;
    };

    struct Note
    {
        double start = 0.0;      // beats from the start of the take
        double length = 0.0;
        int channel = 1, note = 60, velocity = 100;
    };

    /** With the transport stopped, a pause this long starts a new take. */
    static constexpr double idleGapSeconds = 2.0;
    static constexpr size_t maxNotes = 50000;

    void setBeatsPerBar (double beats)  { beatsPerBar = beats > 0.0 ? beats : 4.0; }
    void add (const Event& e);
    void clear();

    bool isEmpty() const                { return notes.empty() && open.empty(); }
    bool isSongAligned() const          { return songTake; }

    /** Finished notes plus still-held notes cut at the last event. */
    std::vector<Note> getNotes() const;

    /** Whole bars for song takes, whole beats for live ones. */
    double getLengthBeats() const;

    /** Changes whenever the take changes (for repainting). */
    uint32_t getRevision() const        { return revision; }
    double getLastEventSeconds() const  { return lastSeconds; }

private:
    struct Open
    {
        double start;
        int velocity;
    };

    void startTake (const Event& first);
    void finish (int channel, int note, double endBeat);

    std::vector<Note> notes;
    std::map<std::pair<int, int>, Open> open;   // (channel, note) -> start
    double beatsPerBar = 4.0;
    double startBeat = 0.0, lastBeat = 0.0, lastSeconds = -1.0e9;
    bool songTake = false;
    bool closed = true;     // the next note starts a new take
    uint32_t revision = 0;
};

} // namespace capture
