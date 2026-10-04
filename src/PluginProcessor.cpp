#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"

ArpForgeProcessor::ArpForgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ArpForge", params::createLayout())
{
    startTimerHz (20);
}

ArpForgeProcessor::~ArpForgeProcessor()
{
    stopTimer();
}

void ArpForgeProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    engine.prepare (currentSampleRate);
    noteInput.reserve (1024);
    noteOutput.reserve (2048);
    outputBuffer.ensureSize (8192);
}

bool ArpForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
        && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

arp::Params ArpForgeProcessor::readParams() const
{
    auto value = [this] (const char* id) { return state.getRawParameterValue (id)->load(); };
    auto index = [&value] (const char* id) { return juce::roundToInt (value (id)); };
    auto flag  = [&value] (const char* id) { return value (id) >= 0.5f; };

    arp::Params p;
    p.style = (arp::Style) juce::jlimit (0, arp::numStyles - 1, index (params::id::style));
    p.sync = flag (params::id::sync);
    p.syncRateBeats = params::rates()[(size_t) juce::jlimit (0, (int) params::rates().size() - 1, index (params::id::rate))].beats;
    p.freeRateMs = value (params::id::freeRate);
    p.gate = value (params::id::gate) / 100.0;
    p.groove = (arp::Groove) index (params::id::groove);
    p.swing = value (params::id::swing) / 100.0;
    p.hold = flag (params::id::hold);
    p.offset = index (params::id::offset);

    const int repeats = index (params::id::repeats);
    p.repeats = repeats >= params::infiniteRepeats ? 0 : repeats;

    p.retrigger = (arp::Retrigger) index (params::id::retrigger);
    p.retriggerBeats = params::retriggerRates()[(size_t) juce::jlimit (0, (int) params::retriggerRates().size() - 1,
                                                                       index (params::id::retriggerRate))].beats;
    p.transposeMode = (arp::TransposeMode) index (params::id::transposeMode);
    p.transposeKey = index (params::id::transposeKey);
    p.transposeScale = index (params::id::transposeScale);
    p.transposeDistance = index (params::id::distance);
    p.transposeSteps = index (params::id::steps);
    p.velocityOn = flag (params::id::velocityOn);
    p.velocityDecayMs = value (params::id::velocityDecay);
    p.velocityTarget = index (params::id::velocityTarget);
    p.velocityRetrigger = flag (params::id::velocityRetrig);
    return p;
}

void ArpForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    bypassed = false;

    engine.setParams (readParams());

    arp::Transport transport;
    if (auto* host = getPlayHead())
    {
        if (const auto position = host->getPosition())
        {
            if (const auto bpm = position->getBpm(); bpm.hasValue() && std::isfinite (*bpm) && *bpm > 1.0)
                transport.bpm = *bpm;

            if (const auto ppq = position->getPpqPosition(); ppq.hasValue() && std::isfinite (*ppq))
            {
                transport.ppqPosition = *ppq;
                transport.hasPosition = true;
            }

            transport.playing = position->getIsPlaying();

            if (const auto sig = position->getTimeSignature())
            {
                timeSigNumerator = juce::jmax (1, sig->numerator);
                timeSigDenominator = juce::jmax (1, sig->denominator);
            }
        }
    }

    // Capture clock: the song position while playing, a free-running one otherwise.
    const double sampleRate = currentSampleRate;
    hostBpm = transport.bpm;
    blockBeatsPerSample = transport.bpm / 60.0 / sampleRate;
    blockPlaying = transport.playing && transport.hasPosition;
    blockBeat = blockPlaying ? transport.ppqPosition : liveBeat;
    blockSeconds = liveSeconds;

    if (blockPlaying != wasPlaying)
        pushCapture (blockPlaying ? capture::CaptureTake::Kind::TransportStart : capture::CaptureTake::Kind::TransportStop, 0);
    wasPlaying = blockPlaying;

    const int numSamples = buffer.getNumSamples();
    int segmentStart = 0;
    outputBuffer.clear();
    noteInput.clear();

    // Notes feed the arpeggiator; everything else (CCs, pitch bend...) passes
    // straight through so the synth still gets it.
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int pos = juce::jlimit (0, juce::jmax (0, numSamples - 1), meta.samplePosition);

        if (msg.isNoteOn())
        {
            noteInput.push_back ({ pos - segmentStart, true, msg.getChannel(), msg.getNoteNumber(), (int) msg.getVelocity() });
        }
        else if (msg.isNoteOff())
        {
            noteInput.push_back ({ pos - segmentStart, false, msg.getChannel(), msg.getNoteNumber(), 0 });
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            runEngine (transport, segmentStart, pos);
            noteOutput.clear();
            engine.releaseAll (0, noteOutput);
            addToOutput (noteOutput, pos);
            outputBuffer.addEvent (msg, pos);
            segmentStart = pos;
        }
        else
        {
            outputBuffer.addEvent (msg, pos);
        }
    }

    runEngine (transport, segmentStart, numSamples);
    midi.swapWith (outputBuffer);

    liveBeat += numSamples * blockBeatsPerSample;
    liveSeconds += numSamples / sampleRate;
    if (blockPlaying)
        lastSongBeat = blockBeat + numSamples * blockBeatsPerSample;

    const juce::SpinLock::ScopedTryLockType lock (snapshotLock);
    if (lock.isLocked())
        engine.fillSnapshot (snapshot);
}

void ArpForgeProcessor::runEngine (const arp::Transport& transport, int start, int end)
{
    if (end <= start)
        return;

    auto segment = transport;
    if (segment.hasPosition)
        segment.ppqPosition += start * segment.bpm / 60.0 / currentSampleRate;

    noteOutput.clear();
    engine.process (segment, end - start, noteInput, noteOutput);
    addToOutput (noteOutput, start);
    noteInput.clear();
}

void ArpForgeProcessor::addToOutput (const std::vector<arp::NoteEvent>& events, int start)
{
    for (const auto& e : events)
    {
        const int channel = juce::jlimit (1, 16, e.channel);
        const auto msg = e.isNoteOn ? juce::MidiMessage::noteOn (channel, e.note, (juce::uint8) e.velocity)
                                    : juce::MidiMessage::noteOff (channel, e.note);
        outputBuffer.addEvent (msg, start + e.sampleOffset);
        pushCapture (e.isNoteOn ? capture::CaptureTake::Kind::NoteOn : capture::CaptureTake::Kind::NoteOff,
                     start + e.sampleOffset, channel, e.note, e.velocity);
    }
}

void ArpForgeProcessor::pushCapture (capture::CaptureTake::Kind kind, int sampleInBlock, int channel, int note, int velocity)
{
    capture::CaptureTake::Event e;
    e.kind = kind;
    e.playing = kind == capture::CaptureTake::Kind::TransportStop ? false : blockPlaying;
    e.beat = kind == capture::CaptureTake::Kind::TransportStop ? lastSongBeat
                                                                : blockBeat + sampleInBlock * blockBeatsPerSample;
    e.seconds = blockSeconds + sampleInBlock / currentSampleRate;
    e.channel = channel;
    e.note = note;
    e.velocity = velocity;

    // A full queue drops the event rather than blocking the audio thread.
    const auto scope = captureFifo.write (1);
    if (scope.blockSize1 > 0)
        captureQueue[(size_t) scope.startIndex1] = e;
}

void ArpForgeProcessor::timerCallback()
{
    take.setBeatsPerBar (getTimeSigNumerator() * 4.0 / getTimeSigDenominator());

    const auto scope = captureFifo.read (captureFifo.getNumReady());
    for (int i = 0; i < scope.blockSize1; ++i)
        take.add (captureQueue[(size_t) (scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        take.add (captureQueue[(size_t) (scope.startIndex2 + i)]);
}

void ArpForgeProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();

    // Bypassed: the incoming notes go through untouched. Close whatever the
    // arpeggiator was playing so nothing hangs.
    if (! bypassed)
    {
        bypassed = true;
        noteOutput.clear();
        engine.releaseAll (0, noteOutput);

        for (const auto& e : noteOutput)
            midi.addEvent (juce::MidiMessage::noteOff (juce::jlimit (1, 16, e.channel), e.note), 0);
    }
}

void ArpForgeProcessor::loadPreset (int index)
{
    const auto& list = presets::all();
    if (index < 0 || index >= (int) list.size())
        return;

    presets::apply (state, list[(size_t) index]);
    state.state.setProperty ("preset", index, nullptr);
}

int ArpForgeProcessor::getPresetIndex() const
{
    return state.state.getProperty ("preset", 0);
}

void ArpForgeProcessor::copySnapshot (arp::Snapshot& dest)
{
    const juce::SpinLock::ScopedLockType lock (snapshotLock);
    dest = snapshot;
}

void ArpForgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ArpForgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ArpForgeProcessor::createEditor()
{
    return new ArpForgeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArpForgeProcessor();
}
