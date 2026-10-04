#include "MidiExport.h"

namespace capture
{

juce::MidiFile toMidiFile (const CaptureTake& take, double bpm, int timeSigNumerator,
                           int timeSigDenominator, const juce::String& trackName)
{
    auto ticks = [] (double beats) { return std::round (beats * midiTicksPerBeat); };

    juce::MidiMessageSequence track;
    track.addEvent (juce::MidiMessage::textMetaEvent (3, trackName), 0.0);
    track.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / juce::jmax (1.0, bpm))), 0.0);
    track.addEvent (juce::MidiMessage::timeSignatureMetaEvent (timeSigNumerator, timeSigDenominator), 0.0);

    // Note-offs go before note-ons at the same tick, so repeated notes don't overlap.
    struct Timed
    {
        double tick;
        bool on;
        juce::MidiMessage message;
    };

    std::vector<Timed> events;
    for (const auto& n : take.getNotes())
    {
        const auto start = ticks (n.start);
        const auto end = juce::jmax (start + 1.0, ticks (n.start + n.length));
        events.push_back ({ start, true, juce::MidiMessage::noteOn (n.channel, n.note, (juce::uint8) juce::jlimit (1, 127, n.velocity)) });
        events.push_back ({ end, false, juce::MidiMessage::noteOff (n.channel, n.note) });
    }

    std::stable_sort (events.begin(), events.end(), [] (const Timed& a, const Timed& b)
                      { return a.tick != b.tick ? a.tick < b.tick : (! a.on && b.on); });

    for (auto& e : events)
        track.addEvent (e.message, e.tick);

    const double endTick = ticks (take.getLengthBeats());
    track.addEvent (juce::MidiMessage::endOfTrack(), juce::jmax (endTick, track.getEndTime()));
    track.updateMatchedPairs();

    juce::MidiFile file;
    file.setTicksPerQuarterNote (midiTicksPerBeat);
    file.addTrack (track);
    return file;
}

bool writeMidiFile (const juce::MidiFile& midi, const juce::File& file)
{
    file.deleteFile();
    juce::FileOutputStream stream (file);
    return stream.openedOk() && midi.writeTo (stream, 1);
}

} // namespace capture
