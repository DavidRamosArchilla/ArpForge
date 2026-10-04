#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "engine/ArpEngine.h"

class ArpForgeProcessor final : public juce::AudioProcessor
{
public:
    ArpForgeProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram (int) override                      {}
    const juce::String getProgramName (int) override           { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Factory presets (message thread). The index is saved with the state. */
    void loadPreset (int index);
    int getPresetIndex() const;

    /** Called from the editor to get what the arpeggiator is doing right now. */
    void copySnapshot (arp::Snapshot& dest);

    juce::AudioProcessorValueTreeState state;

private:
    arp::Params readParams() const;
    void runEngine (const arp::Transport& transport, int start, int end);
    void addToOutput (const std::vector<arp::NoteEvent>& events, int start);

    arp::Engine engine;
    std::vector<arp::NoteEvent> noteInput, noteOutput;
    juce::MidiBuffer outputBuffer;
    bool bypassed = false;

    juce::SpinLock snapshotLock;
    arp::Snapshot snapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpForgeProcessor)
};
