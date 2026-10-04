#pragma once

#include "Theme.h"
#include <juce_audio_basics/juce_audio_basics.h>

class ArpForgeProcessor;

/** Shows the captured take. Drag it into a piano roll or the playlist to
    drop the arpeggio as MIDI notes; click it to save a .mid file. */
class MidiDragTile final : public juce::Component
{
public:
    explicit MidiDragTile (ArpForgeProcessor&);

    /** Called by the editor's timer. */
    void refresh();

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::String describeTake() const;
    juce::String fileName() const;
    juce::MidiFile buildMidi() const;
    void saveAs();

    ArpForgeProcessor& processor;
    std::unique_ptr<juce::FileChooser> chooser;
    uint32_t lastRevision = 0;
    double lastChangeMs = 0.0;
    bool recording = false, hovered = false, dragged = false;
};
