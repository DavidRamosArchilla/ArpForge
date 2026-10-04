#include "Presets.h"
#include "Parameters.h"
#include "engine/ArpTypes.h"

namespace presets
{
namespace
{
    using S = arp::Style;
    namespace id = params::id;

    Value style (S s)            { return { id::style, (float) (int) s }; }
    Value rate (const char* r)   { return { id::rate, (float) params::rateIndex (r) }; }
    Value gate (float percent)   { return { id::gate, percent }; }
    Value swing16()              { return { id::groove, 2.0f }; }
    Value swingAmount (float a)  { return { id::swing, a }; }
    Value swing8()               { return { id::groove, 1.0f }; }
    Value octaves (int steps)    { return { id::steps, (float) steps }; }
} // namespace

const std::vector<Preset>& all()
{
    static const std::vector<Preset> list {
        { "Init", "Classic", {} },
        { "Trance Up & Down", "Classic", { style (S::UpAndDown), gate (70), octaves (1) } },
        { "Dreamy Thumb", "Classic", { style (S::ThumbUp), rate ("1/8"), gate (90), swing8(), swingAmount (58) } },
        { "Converge Keys", "Classic", { style (S::Converge), rate ("1/8T"), gate (80) } },
        { "Random Sparkle", "Classic", { style (S::RandomOther), gate (30), octaves (1),
                                         { id::velocityOn, 1 }, { id::velocityDecay, 2000 }, { id::velocityTarget, 60 } } },

        { "Gallop Pluck", "Rhythmic", { style (S::Gallop), gate (40) } },
        { "Tresillo Pluck", "Rhythmic", { style (S::Tresillo), gate (35), swing16(), swingAmount (56) } },
        { "Octave Runner", "Rhythmic", { style (S::OctaveBounce), gate (50) } },
        { "Alberti Keys", "Rhythmic", { style (S::Alberti), rate ("1/8"), gate (85) } },
        { "Stutter Lead", "Rhythmic", { style (S::Stutter), gate (45) } },
        { "Syncopated Bells", "Rhythmic", { style (S::Syncopated), gate (60), octaves (1) } },
        { "Dotted Echo", "Rhythmic", { style (S::DottedEighths), gate (55), octaves (1) } },
        { "Pedal Drive", "Rhythmic", { style (S::Pedal), gate (45) } },
        { "Cascade 3:4", "Rhythmic", { style (S::Cascade), gate (50) } },
        { "Ping Pong", "Rhythmic", { style (S::PingPong), gate (40), swing16(), swingAmount (54) } },

        { "House Stabs", "Chord Rhythms", { style (S::OffbeatStabs), gate (35) } },
        { "Tresillo Chords", "Chord Rhythms", { style (S::TresilloChords), gate (80) } },
        { "Charleston Keys", "Chord Rhythms", { style (S::Charleston), gate (85), swing8(), swingAmount (60) } },
        { "Clave Keys", "Chord Rhythms", { style (S::Clave), gate (60) } },
        { "Reggaeton Chords", "Chord Rhythms", { style (S::Dembow), gate (55) } },
        { "Pulse Chords", "Chord Rhythms", { style (S::PulseAccents), gate (45) } },
        { "Trance Gate Pad", "Chord Rhythms", { style (S::TranceGate), gate (70) } },
        { "Piano Comp", "Chord Rhythms", { style (S::PianoComp), gate (90), swing16(), swingAmount (55) } },
        { "Piano Ballad", "Chord Rhythms", { style (S::Ballad), rate ("1/8"), gate (95) } },
        { "Reggae Skank", "Chord Rhythms", { style (S::Skank), gate (30) } },
        { "Gallop Chords", "Chord Rhythms", { style (S::GallopChords), gate (50) } },
        { "Stutter Chords", "Chord Rhythms", { style (S::StutterChords), gate (60) } },
    };
    return list;
}

void apply (juce::AudioProcessorValueTreeState& state, const Preset& preset)
{
    auto set = [] (juce::RangedAudioParameter& p, float normalised)
    {
        p.beginChangeGesture();
        p.setValueNotifyingHost (normalised);
        p.endChangeGesture();
    };

    for (auto* p : state.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (ranged->getParameterID() != params::id::hold)
                set (*ranged, ranged->getDefaultValue());

    for (const auto& v : preset.values)
        if (auto* p = state.getParameter (v.id))
            set (*p, p->convertTo0to1 (v.value));
}

} // namespace presets
