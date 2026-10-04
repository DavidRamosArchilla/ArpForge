// Dev tool: loads a built VST3 the way a DAW does, feeds it a chord and
// prints the MIDI it sends out.
//   HostTest.exe <path-to-ArpForge.vst3>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::puts ("usage: HostTest <plugin.vst3>");
        return 2;
    }

    const juce::ScopedJuceInitialiser_GUI gui;
    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> found;
    format.findAllTypesForFile (found, argv[1]);

    if (found.isEmpty())
    {
        std::puts ("no plugin found");
        return 1;
    }

    const auto& desc = *found[0];
    std::printf ("plugin: %s  instrument=%d  in=%d out=%d\n", desc.name.toRawUTF8(), (int) desc.isInstrument,
                 desc.numInputChannels, desc.numOutputChannels);

    juce::String error;
    auto plugin = format.createInstanceFromDescription (desc, 48000.0, 512, error);
    if (plugin == nullptr)
    {
        std::printf ("load failed: %s\n", error.toRawUTF8());
        return 1;
    }

    std::printf ("acceptsMidi=%d producesMidi=%d\n", (int) plugin->acceptsMidi(), (int) plugin->producesMidi());
    plugin->prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> audio (juce::jmax (2, plugin->getTotalNumOutputChannels()), 512);
    juce::MidiBuffer midi;
    for (int note : { 59, 62, 67 })
        midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);

    int events = 0;
    for (int block = 0; block < 100; ++block)
    {
        audio.clear();
        plugin->processBlock (audio, midi);

        for (const auto meta : midi)
            if (events++ < 12)
                std::printf ("block %3d +%3d  %s\n", block, meta.samplePosition, meta.getMessage().getDescription().toRawUTF8());

        midi.clear();
    }

    std::printf ("total MIDI events out: %d\n", events);
    plugin->releaseResources();
    return events > 0 ? 0 : 1;
}
