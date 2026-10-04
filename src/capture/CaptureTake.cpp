#include "CaptureTake.h"
#include <algorithm>
#include <cmath>

namespace capture
{

void CaptureTake::clear()
{
    notes.clear();
    open.clear();
    startBeat = lastBeat = 0.0;
    ++revision;
}

void CaptureTake::startTake (const Event& first)
{
    clear();
    songTake = first.playing;
    closed = false;
    // Song takes start on the bar line so the clip drops in time.
    startBeat = songTake ? std::floor (first.beat / beatsPerBar + 1.0e-9) * beatsPerBar : first.beat;
    lastBeat = first.beat;
}

void CaptureTake::finish (int channel, int note, double endBeat)
{
    const auto it = open.find ({ channel, note });
    if (it == open.end())
        return;

    if (notes.size() < maxNotes)
        notes.push_back ({ it->second.start - startBeat, std::max (endBeat - it->second.start, 1.0e-3),
                           channel, note, it->second.velocity });
    open.erase (it);
}

void CaptureTake::add (const Event& e)
{
    switch (e.kind)
    {
        case Kind::TransportStart:
            // The next note starts a fresh, bar-aligned take.
            closed = true;
            break;

        case Kind::TransportStop:
            // End the song take at the stop position; the host's note-offs
            // that follow belong to a different clock and are ignored.
            for (auto it = open.begin(); it != open.end();)
            {
                const auto key = it++->first;
                finish (key.first, key.second, std::max (e.beat, lastBeat));
            }
            closed = true;
            break;

        case Kind::NoteOn:
        {
            const bool paused = ! songTake && open.empty() && e.seconds - lastSeconds > idleGapSeconds;
            if (closed || paused || songTake != e.playing)
                startTake (e);

            finish (e.channel, e.note, e.beat);   // same key retriggered
            open[{ e.channel, e.note }] = { e.beat, e.velocity };
            lastBeat = std::max (lastBeat, e.beat);
            break;
        }

        case Kind::NoteOff:
            if (! closed && songTake == e.playing)
            {
                finish (e.channel, e.note, e.beat);
                lastBeat = std::max (lastBeat, e.beat);
            }
            break;
    }

    lastSeconds = e.seconds;
    ++revision;
}

std::vector<CaptureTake::Note> CaptureTake::getNotes() const
{
    auto result = notes;

    for (const auto& [key, o] : open)
        result.push_back ({ o.start - startBeat, std::max (lastBeat - o.start, 0.25), key.first, key.second, o.velocity });

    std::sort (result.begin(), result.end(), [] (const Note& a, const Note& b)
               { return a.start != b.start ? a.start < b.start : a.note < b.note; });
    return result;
}

double CaptureTake::getLengthBeats() const
{
    double end = 0.0;
    for (const auto& n : getNotes())
        end = std::max (end, n.start + n.length);

    const double unit = songTake ? beatsPerBar : 1.0;
    return std::max (unit, std::ceil (end / unit - 1.0e-6) * unit);
}

} // namespace capture
