#include "ArpEngine.h"
#include "Patterns.h"
#include "Scales.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <numeric>

namespace arp
{
namespace
{
    constexpr int never = INT_MAX;
    constexpr double gridEpsilon = 1.0e-7;

    // When the arp starts and a grid line is this close (as a fraction of a
    // step), wait for it instead of playing the first note straight away.
    constexpr double startWindow = 0.125;
} // namespace

Engine::Engine() : rng (std::random_device {}()) {}

void Engine::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    physical.reserve (128);
    active.reserve (128);
    sorted.reserve (128);
    sequence.reserve (512);
    sounding.reserve (512);
    reset();
}

void Engine::setParams (const Params& newParams)
{
    const bool styleChanged = newParams.style != params.style;
    const bool holdReleased = params.hold && ! newParams.hold;

    params = newParams;
    params.transposeScale = std::clamp (params.transposeScale, 0, (int) scales().size() - 1);
    params.transposeKey = std::clamp (params.transposeKey, 0, 11);
    params.swing = std::clamp (params.swing, 0.5, 0.75);
    params.offset = std::max (0, params.offset);
    params.transposeSteps = std::max (0, params.transposeSteps);

    if (styleChanged)
        sequenceDirty = true;

    if (holdReleased)
        syncActiveWithHold();
}

void Engine::reset()
{
    physical.clear();
    active.clear();
    sorted.clear();
    sequence.clear();
    sounding.clear();
    sequenceDirty = true;
    running = finished = immediateStep = false;
    position = transposeIndex = cycles = 0;
    engineBeat = engineSeconds = gridOffset = 0.0;
    lastDisplayStep = -1;
}

//==============================================================================
void Engine::process (const Transport& transport, int numSamples,
                      const std::vector<NoteEvent>& input, std::vector<NoteEvent>& out)
{
    if (numSamples <= 0)
        return;

    const double bpm = transport.bpm > 1.0 ? transport.bpm : 120.0;
    beatsPerSample = bpm / 60.0 / sampleRate;
    blockStartBeat = engineBeat;
    blockStartSeconds = engineSeconds;

    hostGrid = transport.playing && transport.hasPosition;
    if (hostGrid)
        gridOffset = transport.ppqPosition - engineBeat;

    // Re-derive the next grid step from the song position every block, which
    // absorbs loops, jumps, tempo changes and rate changes.
    if (running && params.sync)
        scheduleGridStep (firstGridIndexAtOrAfter (hostPosAt (0) - gridEpsilon));

    size_t next = 0;
    int cursor = 0;

    for (;;)
    {
        const int eventSample = next < input.size()
                                  ? std::clamp (input[next].sampleOffset, cursor, numSamples - 1)
                                  : never;
        const int stepSample = running ? sampleFor (nextStepBeat, cursor, numSamples) : never;

        int offSample = never;
        for (const auto& s : sounding)
            offSample = std::min (offSample, sampleFor (s.offBeat, cursor, numSamples));

        const int now = std::min ({ eventSample, stepSample, offSample });
        if (now >= numSamples)
            break;

        cursor = now;

        // Order within one sample: note-offs, then incoming notes, then the step.
        emitNoteOffsUpTo (now, numSamples, out);

        if (eventSample == now)
        {
            groupHadNewNote = false;
            groupStartedEmpty = active.empty();

            while (next < input.size() && std::clamp (input[next].sampleOffset, cursor, numSamples - 1) <= now)
                handleNote (input[next++]);

            afterEventGroup (now);
        }

        if (running && sampleFor (nextStepBeat, now, numSamples) <= now)
            fireStep (now, out);
    }

    engineBeat += numSamples * beatsPerSample;
    engineSeconds += numSamples / sampleRate;
}

void Engine::releaseAll (int sampleOffset, std::vector<NoteEvent>& out)
{
    for (const auto& s : sounding)
        out.push_back ({ sampleOffset, false, s.channel, s.note, 0 });

    sounding.clear();
    physical.clear();
    active.clear();
    sequenceDirty = true;
    running = false;
}

//==============================================================================
int Engine::sampleFor (double beat, int notBefore, int numSamples) const
{
    const double s = (beat - blockStartBeat) / beatsPerSample;

    if (s >= (double) numSamples)
        return never;

    if (s <= (double) notBefore)
        return notBefore;

    return std::max ((int) std::ceil (s - 1.0e-6), notBefore);
}

double Engine::stepLength() const
{
    const double len = params.sync ? params.syncRateBeats
                                   : params.freeRateMs * 0.001 * sampleRate * beatsPerSample;
    return std::max (len, 1.0e-4);
}

double Engine::warp (double p) const
{
    if (! params.sync || params.groove == Groove::Straight)
        return p;

    const double half = params.groove == Groove::Swing8 ? 0.5 : 0.25;
    const double period = 2.0 * half;
    const double split = period * params.swing;
    const double base = std::floor (p / period) * period;
    const double f = p - base;

    return f < half ? base + f * (split / half)
                    : base + split + (f - half) * ((period - split) / half);
}

double Engine::unwarp (double p) const
{
    if (! params.sync || params.groove == Groove::Straight)
        return p;

    const double half = params.groove == Groove::Swing8 ? 0.5 : 0.25;
    const double period = 2.0 * half;
    const double split = period * params.swing;
    const double base = std::floor (p / period) * period;
    const double f = p - base;

    return f < split ? base + f * (half / split)
                     : base + half + (f - split) * (half / (period - split));
}

int64_t Engine::firstGridIndexAtOrAfter (double hostPos) const
{
    return (int64_t) std::ceil (unwarp (hostPos) / stepLength() - 1.0e-9);
}

void Engine::scheduleGridStep (int64_t index)
{
    nextGridIndex = index;
    nextStepBeat = warp ((double) index * stepLength()) - gridOffset;
}

//==============================================================================
void Engine::handleNote (const NoteEvent& e)
{
    auto sameKey = [&e] (const HeldNote& h) { return h.note == e.note && h.channel == e.channel; };

    if (e.isNoteOn && e.velocity > 0)
    {
        // With Hold on, a new note after every key was released starts a new latch.
        if (params.hold && physical.empty())
            active.clear();

        physical.erase (std::remove_if (physical.begin(), physical.end(), sameKey), physical.end());
        physical.push_back ({ e.note, e.channel, e.velocity, orderCounter });

        auto it = std::find_if (active.begin(), active.end(), sameKey);
        if (it == active.end())
            active.push_back ({ e.note, e.channel, e.velocity, orderCounter });
        else
            it->velocity = e.velocity;

        ++orderCounter;
        groupHadNewNote = true;
    }
    else
    {
        physical.erase (std::remove_if (physical.begin(), physical.end(), sameKey), physical.end());

        if (! params.hold)
            active.erase (std::remove_if (active.begin(), active.end(), sameKey), active.end());
    }

    sequenceDirty = true;
}

void Engine::afterEventGroup (int sample)
{
    if (active.empty())
    {
        running = false;
        return;
    }

    if (! running || groupStartedEmpty)
    {
        start (sample);
        return;
    }

    if (groupHadNewNote && (params.retrigger == Retrigger::Note || finished))
        resetPattern (sample);
}

void Engine::syncActiveWithHold()
{
    active.erase (std::remove_if (active.begin(), active.end(),
                                  [this] (const HeldNote& a)
                                  {
                                      return std::none_of (physical.begin(), physical.end(),
                                                           [&a] (const HeldNote& p) { return p.note == a.note && p.channel == a.channel; });
                                  }),
                  active.end());
    sequenceDirty = true;

    if (active.empty())
        running = false;
}

void Engine::start (int sample)
{
    running = true;
    resetPattern (sample);
    velocityRampStart = secondsAt (sample);
    lastDisplayStep = -1;

    // Stopped transport: the grid starts with the first note.
    if (! hostGrid)
        gridOffset = -beatAt (sample);

    const double here = hostPosAt (sample);
    lastRetriggerBlock = (int64_t) std::floor ((here + 1.0e-6) / std::max (params.retriggerBeats, 1.0e-3));

    if (params.sync)
    {
        const double len = stepLength();
        const int64_t k = firstGridIndexAtOrAfter (here - gridEpsilon);

        if (warp ((double) k * len) - here <= startWindow * len)
        {
            immediateStep = false;
            scheduleGridStep (k);
            return;
        }
    }

    immediateStep = true;
    nextStepBeat = beatAt (sample);
}

void Engine::resetPattern (int sample)
{
    position = 0;
    transposeIndex = 0;
    cycles = 0;
    finished = false;

    if (params.velocityRetrigger)
        velocityRampStart = secondsAt (sample);

    if (params.style == Style::RandomOther && ! sequenceDirty)
        shuffleSequence();
}

//==============================================================================
void Engine::fireStep (int sample, std::vector<NoteEvent>& out)
{
    if (sequenceDirty)
        rebuildSequence();

    const double len = stepLength();
    const bool onGrid = params.sync && ! immediateStep;
    const double stepHostPos = onGrid ? (double) nextGridIndex * len : hostPosAt (sample);

    if (params.retrigger == Retrigger::Beat)
    {
        const auto block = (int64_t) std::floor ((stepHostPos + 1.0e-6) / std::max (params.retriggerBeats, 1.0e-3));
        if (block != lastRetriggerBlock)
        {
            lastRetriggerBlock = block;
            resetPattern (sample);
        }
    }

    const int n = (int) sequence.size();

    if (! finished && n > 0 && ! sorted.empty())
    {
        if (position >= n)
            position = 0;
        if (transposeIndex > params.transposeSteps)
            transposeIndex = 0;

        const int slot = params.style == Style::Random ? (int) (rng() % (unsigned) n)
                                                       : (position + params.offset) % n;

        double offBeat = onGrid ? warp (stepHostPos + params.gate * len) - gridOffset
                                : beatAt (sample) + params.gate * len;
        offBeat = std::max (offBeat, beatAt (sample) + beatsPerSample);

        auto play = [&] (const HeldNote& h)
        {
            const int pitch = transposeNote (h.note);
            if (pitch < 0 || pitch > 127)
                return;

            // A long gate can still be holding this pitch: end it first.
            for (auto it = sounding.begin(); it != sounding.end();)
            {
                if (it->note == pitch && it->channel == h.channel)
                {
                    out.push_back ({ sample, false, h.channel, pitch, 0 });
                    it = sounding.erase (it);
                }
                else
                {
                    ++it;
                }
            }

            out.push_back ({ sample, true, h.channel, pitch, velocityFor (h.velocity, sample) });
            sounding.push_back ({ pitch, h.channel, offBeat });
        };

        const int value = sequence[(size_t) slot];
        if (value == chordStep)
            for (const auto& h : sorted)
                play (h);
        else
            play (sorted[(size_t) value]);

        lastDisplayStep = ((slot - params.offset) % n + n) % n;
        ++stepCounter;

        if (++position >= n)
        {
            position = 0;

            if (++transposeIndex > params.transposeSteps)
            {
                transposeIndex = 0;
                ++cycles;

                if (params.repeats > 0 && cycles >= params.repeats)
                    finished = true;
            }

            if (params.style == Style::RandomOther)
                shuffleSequence();
        }
    }

    // Schedule the next step.
    if (params.sync)
    {
        if (immediateStep)
            scheduleGridStep (firstGridIndexAtOrAfter (hostPosAt (sample) + gridEpsilon));
        else
            scheduleGridStep (nextGridIndex + 1);
    }
    else
    {
        nextStepBeat = beatAt (sample) + len;
    }

    immediateStep = false;

    const double earliest = beatAt (sample) + 0.5 * beatsPerSample;
    while (nextStepBeat < earliest)
    {
        if (params.sync)
            scheduleGridStep (nextGridIndex + 1);
        else
            nextStepBeat += len;
    }
}

void Engine::emitNoteOffsUpTo (int sample, int numSamples, std::vector<NoteEvent>& out)
{
    for (auto it = sounding.begin(); it != sounding.end();)
    {
        if (sampleFor (it->offBeat, sample, numSamples) <= sample)
        {
            out.push_back ({ sample, false, it->channel, it->note, 0 });
            it = sounding.erase (it);
        }
        else
        {
            ++it;
        }
    }
}

void Engine::rebuildSequence()
{
    sorted = active;
    std::sort (sorted.begin(), sorted.end(), [] (const HeldNote& a, const HeldNote& b)
               { return a.note != b.note ? a.note < b.note : a.channel < b.channel; });

    const int n = (int) sorted.size();
    sequence = buildPattern (params.style, n);

    if (params.style == Style::PlayOrder)
    {
        sequence.resize ((size_t) n);
        std::iota (sequence.begin(), sequence.end(), 0);
        std::sort (sequence.begin(), sequence.end(), [this] (int a, int b)
                   { return sorted[(size_t) a].order < sorted[(size_t) b].order; });
    }

    sequenceDirty = false;

    if (params.style == Style::RandomOther || params.style == Style::RandomOnce)
        shuffleSequence();
}

void Engine::shuffleSequence()
{
    std::shuffle (sequence.begin(), sequence.end(), rng);
}

int Engine::transposeNote (int note) const
{
    const int amount = transposeIndex * params.transposeDistance;

    if (amount == 0)
        return note;

    if (params.transposeMode == TransposeMode::Key)
        return transposeInKey (note, amount, params.transposeKey, params.transposeScale);

    return note + amount;
}

int Engine::velocityFor (int noteVelocity, int sample) const
{
    if (! params.velocityOn)
        return noteVelocity;

    const double elapsedMs = (secondsAt (sample) - velocityRampStart) * 1000.0;
    const double t = params.velocityDecayMs <= 0.0 ? 1.0 : std::min (1.0, elapsedMs / params.velocityDecayMs);
    const double v = noteVelocity + (params.velocityTarget - noteVelocity) * t;

    return std::clamp ((int) std::lround (v), 1, 127);
}

//==============================================================================
void Engine::fillSnapshot (Snapshot& s)
{
    if (sequenceDirty)
        rebuildSequence();

    const int n = (int) sequence.size();
    s.active = running && ! finished && ! active.empty();
    s.numSteps = std::min (n, Snapshot::maxSteps);

    for (int i = 0; i < s.numSteps; ++i)
    {
        auto& step = s.steps[(size_t) i];
        const int value = sequence[(size_t) ((i + params.offset) % n)];
        step.count = 0;

        if (value == chordStep)
        {
            for (const auto& h : sorted)
                if (step.count < Snapshot::maxNotesPerStep)
                    step.notes[step.count++] = (uint8_t) h.note;
        }
        else
        {
            step.notes[0] = (uint8_t) sorted[(size_t) value].note;
            step.count = 1;
        }
    }

    s.currentStep = s.active && lastDisplayStep < s.numSteps ? lastDisplayStep : -1;
    s.transposeIndex = transposeIndex;
    s.transposeSteps = params.transposeSteps;
    s.stepCounter = stepCounter;
}

} // namespace arp
