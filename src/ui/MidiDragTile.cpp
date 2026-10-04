#include "MidiDragTile.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include "../capture/MidiExport.h"

using namespace theme;

MidiDragTile::MidiDragTile (ArpForgeProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void MidiDragTile::refresh()
{
    const auto& take = processor.getCapture();
    const double now = juce::Time::getMillisecondCounterHiRes();

    if (take.getRevision() != lastRevision)
    {
        lastRevision = take.getRevision();
        lastChangeMs = now;
        repaint();
    }

    const bool isRecording = now - lastChangeMs < 400.0 && ! take.isEmpty();
    if (isRecording != recording)
    {
        recording = isRecording;
        repaint();
    }
}

juce::String MidiDragTile::describeTake() const
{
    const auto& take = processor.getCapture();
    if (take.isEmpty())
        return "PLAY TO CAPTURE";

    const double beats = take.getLengthBeats();

    if (take.isSongAligned())
    {
        const double beatsPerBar = processor.getTimeSigNumerator() * 4.0 / processor.getTimeSigDenominator();
        const int bars = juce::roundToInt (beats / beatsPerBar);
        return "DRAG MIDI  " + juce::String (bars) + (bars == 1 ? " BAR" : " BARS");
    }

    const int wholeBeats = juce::roundToInt (beats);
    return "DRAG MIDI  " + juce::String (wholeBeats) + (wholeBeats == 1 ? " BEAT" : " BEATS");
}

juce::String MidiDragTile::fileName() const
{
    juce::String style = "Arp";
    if (auto* p = processor.state.getParameter (params::id::style))
        style = p->getCurrentValueAsText();

    return juce::File::createLegalFileName ("ArpForge " + style + " " + juce::String (juce::roundToInt (processor.getCaptureBpm())) + "bpm");
}

juce::MidiFile MidiDragTile::buildMidi() const
{
    return capture::toMidiFile (processor.getCapture(), processor.getCaptureBpm(),
                                processor.getTimeSigNumerator(), processor.getTimeSigDenominator(), "ArpForge");
}

void MidiDragTile::paint (juce::Graphics& g)
{
    const bool empty = processor.getCapture().isEmpty();
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    const float corner = b.getHeight() * 0.5f;

    g.setColour (hovered && ! empty ? colour::trackHover : colour::track);
    g.fillRoundedRectangle (b, corner);

    // Icon: a little clip with notes, or a recording dot while capturing.
    const auto icon = b.withWidth (b.getHeight()).reduced (6.0f);
    if (recording)
    {
        g.setColour (colour::accent);
        g.fillEllipse (icon.withSizeKeepingCentre (7.0f, 7.0f));
    }
    else
    {
        g.setColour (empty ? colour::textFaint : colour::accentHot);
        const float w = icon.getWidth() / 3.0f;
        for (int i = 0; i < 3; ++i)
        {
            const float h = icon.getHeight() * (0.45f + 0.18f * (float) ((i * 2) % 3));
            g.fillRoundedRectangle (icon.getX() + w * (float) i + 0.5f, icon.getBottom() - h, w - 1.5f, h, 1.0f);
        }
    }

    g.setColour (empty ? colour::textFaint : (hovered ? colour::text : colour::textDim));
    g.setFont (caps (10.0f));
    g.drawText (describeTake(), b.withTrimmedLeft (b.getHeight()).withTrimmedRight (10.0f),
                juce::Justification::centredLeft, false);
}

void MidiDragTile::mouseEnter (const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

void MidiDragTile::mouseExit (const juce::MouseEvent&)
{
    hovered = false;
    repaint();
}

void MidiDragTile::mouseDrag (const juce::MouseEvent& e)
{
    if (dragged || processor.getCapture().isEmpty() || e.getDistanceFromDragStart() < 5)
        return;

    dragged = true;

    // Each drag gets its own file: hosts may still be reading the previous one.
    auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ArpForge");
    folder.createDirectory();
    const auto file = folder.getNonexistentChildFile (fileName(), ".mid", false);

    if (capture::writeMidiFile (buildMidi(), file))
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

void MidiDragTile::mouseUp (const juce::MouseEvent& e)
{
    const bool wasDrag = dragged;
    dragged = false;

    if (! wasDrag && e.mouseWasClicked() && ! processor.getCapture().isEmpty())
        saveAs();
}

void MidiDragTile::saveAs()
{
    const auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    chooser = std::make_unique<juce::FileChooser> ("Save the captured arpeggio", folder.getChildFile (fileName() + ".mid"), "*.mid");

    const auto midi = buildMidi();
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [midi] (const juce::FileChooser& fc)
                          {
                              const auto result = fc.getResult();
                              if (result != juce::File())
                                  capture::writeMidiFile (midi, result.withFileExtension ("mid"));
                          });
}
