// Dev tool: renders the editor with a chord playing and saves it as a PNG,
// so the UI can be checked without a DAW.
//   UiSnapshot.exe <output.png> [scale] [preset index]

#include "../src/PluginProcessor.h"
#include "../src/Parameters.h"

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
    return 0;
}
