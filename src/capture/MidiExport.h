#pragma once

#include "CaptureTake.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace capture
{

/** Ticks per quarter note in exported files (FL Studio, Ableton and others read it fine). */
constexpr int midiTicksPerBeat = 960;

/** Builds a type-1 MIDI file with tempo, time signature and the take's notes. */
juce::MidiFile toMidiFile (const CaptureTake& take, double bpm, int timeSigNumerator,
                           int timeSigDenominator, const juce::String& trackName);

bool writeMidiFile (const juce::MidiFile& midi, const juce::File& file);

} // namespace capture
