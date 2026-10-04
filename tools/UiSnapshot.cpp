// Dev tool: renders the editor with a chord playing and saves it as a PNG,
// so the UI can be checked without a DAW.
//   UiSnapshot.exe <output.png> [scale] [preset index] [capture.mid]
// With a .mid path it also plays the transport, captures the arpeggio and
// writes it, then prints the file's notes.

#include "../src/PluginProcessor.h"
#include "../src/Parameters.h"
#include "../src/capture/MidiExport.h"

namespace
{
    /** A transport playing at 120 BPM in 4/4 from a given song position. */
    struct FakePlayHead final : juce::AudioPlayHead
    {
        double ppq = 0.0;
        bool playing = false;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setBpm (120.0);
            info.setTimeSignature (TimeSignature { 4, 4 });
            info.setIsPlaying (playing);
            info.setPpqPosition (ppq);
            return info;
        }
    };
} // namespace

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::puts ("usage: UiSnapshot <output.png> [scale]");
        return 2;
    }

    const juce::ScopedJuceInitialiser_GUI gui;
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 1.0f;

    ArpForgeProcessor processor;
    auto set = [&processor] (const char* id, float value)
    {
        auto* p = processor.state.getParameter (id);
        p->setValueNotifyingHost (p->convertTo0to1 (value));
    };
    if (argc > 3)
    {
        processor.loadPreset (std::atoi (argv[3]));
    }
    else
    {
        set (params::id::style, 4.0f);          // Up & Down
        set (params::id::steps, 1.0f);
        set (params::id::groove, 2.0f);
        set (params::id::velocityOn, 1.0f);
    }

    const bool captureTest = argc > 4;
    FakePlayHead playHead;
    playHead.playing = captureTest;
    playHead.ppq = 2.5;   // start mid-bar: the clip should still begin on the bar
    processor.setPlayHead (&playHead);
    processor.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> audio (2, 512);
    juce::MidiBuffer midi;
    for (int note : { 48, 60, 64, 67 })
        midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);

    // Run until a few steps into the pattern.
    for (int block = 0; block < 60; ++block)
    {
        processor.processBlock (audio, midi);
        midi.clear();
        playHead.ppq += 512.0 * 2.0 / 48000.0;
    }

    if (captureTest)
    {
        // Release the chord and stop, as a host would.
        for (int note : { 48, 60, 64, 67 })
            midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
        processor.processBlock (audio, midi);
        playHead.playing = false;
        midi.clear();
        processor.processBlock (audio, midi);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (juce::roundToInt (820 * scale), juce::roundToInt (500 * scale));
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat().writeImageToStream (image, stream);
    std::printf ("wrote %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());

    if (captureTest)
    {
        const auto& take = processor.getCapture();
        const juce::File midiFile (juce::File::getCurrentWorkingDirectory().getChildFile (argv[4]));
        capture::writeMidiFile (capture::toMidiFile (take, 120.0, 4, 4, "ArpForge"), midiFile);

        juce::FileInputStream in (midiFile);
        juce::MidiFile readBack;
        readBack.readFrom (in);
        const auto* track = readBack.getTrack (0);
        std::printf ("capture: %s take, %.2f beats, %d events, %d tpq\n", take.isSongAligned() ? "song" : "live",
                     take.getLengthBeats(), track != nullptr ? track->getNumEvents() : 0, readBack.getTimeFormat());

        for (int i = 0, shown = 0; track != nullptr && i < track->getNumEvents() && shown < 10; ++i)
        {
            const auto& m = track->getEventPointer (i)->message;
            if (m.isNoteOn())
            {
                std::printf ("  tick %6.0f  %s vel %d  len %.0f\n", m.getTimeStamp(),
                             juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 5).toRawUTF8(),
                             m.getVelocity(), track->getTimeOfMatchingKeyUp (i) - m.getTimeStamp());
                ++shown;
            }
        }
    }

    return 0;
}
