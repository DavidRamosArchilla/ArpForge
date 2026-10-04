#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "engine/ArpEngine.h"
#include "capture/CaptureTake.h"

class ArpForgeProcessor final : public juce::AudioProcessor,
                                private juce::Timer
{
public:
    ArpForgeProcessor();
    ~ArpForgeProcessor() override;

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

    /** Message thread: the latest take of generated notes, for MIDI export. */
    const capture::CaptureTake& getCapture() const { return take; }
    double getCaptureBpm() const                   { return hostBpm.load(); }
    int getTimeSigNumerator() const                { return timeSigNumerator.load(); }
    int getTimeSigDenominator() const              { return timeSigDenominator.load(); }

    juce::AudioProcessorValueTreeState state;

private:
    void timerCallback() override;
    void pushCapture (capture::CaptureTake::Kind kind, int sampleInBlock, int channel = 1, int note = 0, int velocity = 0);

    arp::Params readParams() const;
    void runEngine (const arp::Transport& transport, int start, int end);
    void addToOutput (const std::vector<arp::NoteEvent>& events, int start);

    arp::Engine engine;
    double currentSampleRate = 44100.0;
    std::vector<arp::NoteEvent> noteInput, noteOutput;
    juce::MidiBuffer outputBuffer;
    bool bypassed = false;

    juce::SpinLock snapshotLock;
    arp::Snapshot snapshot;

    // Capture: the audio thread queues events, the message thread builds the take.
    static constexpr int captureQueueSize = 8192;
    juce::AbstractFifo captureFifo { captureQueueSize };
    std::vector<capture::CaptureTake::Event> captureQueue = std::vector<capture::CaptureTake::Event> (captureQueueSize);
    capture::CaptureTake take;
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<int> timeSigNumerator { 4 }, timeSigDenominator { 4 };

    // Audio-thread clock for capture timestamps.
    bool wasPlaying = false;
    double liveBeat = 0.0, liveSeconds = 0.0, lastSongBeat = 0.0;
    double blockBeat = 0.0, blockSeconds = 0.0, blockBeatsPerSample = 0.0;
    bool blockPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpForgeProcessor)
};
