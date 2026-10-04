#include "Parameters.h"
#include "engine/Scales.h"

namespace params
{
namespace
{
    juce::StringArray namesOf (const auto& list)
    {
        juce::StringArray names;
        for (const auto& item : list)
            names.add (item.name);
        return names;
    }

    juce::NormalisableRange<float> skewedRange (float min, float max, float centre, float step)
    {
        juce::NormalisableRange<float> range (min, max, step);
        range.setSkewForCentre (centre);
        return range;
    }

    auto msText()
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int)
            {
                return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s"
                                    : juce::String (juce::roundToInt (v)) + " ms";
            });
    }

    auto percentText()
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (juce::roundToInt (v)) + "%"; });
    }
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;

    juce::StringArray scaleNames;
    for (const auto& s : arp::scales())
        scaleNames.add (s.name);

    auto signedText = AudioParameterIntAttributes().withStringFromValueFunction (
        [] (int v, int) { return v > 0 ? "+" + String (v) : String (v); });

    auto repeatsText = AudioParameterIntAttributes()
        .withStringFromValueFunction ([] (int v, int) { return v >= infiniteRepeats ? String (CharPointer_UTF8 ("\xe2\x88\x9e")) : String (v); })
        .withValueFromStringFunction ([] (const String& s)
                                      {
                                          const auto t = s.trim();
                                          return t.startsWithIgnoreCase ("inf") || t == CharPointer_UTF8 ("\xe2\x88\x9e")
                                                   ? infiniteRepeats : t.getIntValue();
                                      });

    return {
        std::make_unique<AudioParameterChoice> (ParameterID { id::style, 1 }, "Style", styleNames(), 0),

        std::make_unique<AudioParameterBool>   (ParameterID { id::sync, 1 }, "Sync", true),
        std::make_unique<AudioParameterChoice> (ParameterID { id::rate, 1 }, "Rate", namesOf (rates()), defaultRateIndex),
        std::make_unique<AudioParameterFloat>  (ParameterID { id::freeRate, 1 }, "Free Rate",
                                                skewedRange (10.0f, 2000.0f, 200.0f, 1.0f), 125.0f, msText()),
        std::make_unique<AudioParameterFloat>  (ParameterID { id::gate, 1 }, "Gate",
                                                NormalisableRange<float> (1.0f, 200.0f, 1.0f), 50.0f, percentText()),

        std::make_unique<AudioParameterChoice> (ParameterID { id::groove, 1 }, "Groove",
                                                StringArray { "Straight", "Swing 8", "Swing 16" }, 0),
        std::make_unique<AudioParameterFloat>  (ParameterID { id::swing, 1 }, "Swing",
                                                NormalisableRange<float> (50.0f, 75.0f, 1.0f), 60.0f, percentText()),

        std::make_unique<AudioParameterBool>   (ParameterID { id::hold, 1 }, "Hold", false),
        std::make_unique<AudioParameterInt>    (ParameterID { id::offset, 1 }, "Offset", 0, 24, 0),
        std::make_unique<AudioParameterInt>    (ParameterID { id::repeats, 1 }, "Repeats", 1, infiniteRepeats, infiniteRepeats, repeatsText),

        std::make_unique<AudioParameterChoice> (ParameterID { id::retrigger, 1 }, "Retrigger",
                                                StringArray { "Off", "Note", "Beat" }, 0),
        std::make_unique<AudioParameterChoice> (ParameterID { id::retriggerRate, 1 }, "Retrigger Rate",
                                                namesOf (retriggerRates()), 4),

        std::make_unique<AudioParameterChoice> (ParameterID { id::transposeMode, 1 }, "Transpose Mode",
                                                StringArray { "Shift", "Key" }, 0),
        std::make_unique<AudioParameterChoice> (ParameterID { id::transposeKey, 1 }, "Key", keyNames(), 0),
        std::make_unique<AudioParameterChoice> (ParameterID { id::transposeScale, 1 }, "Scale", scaleNames, 0),
        std::make_unique<AudioParameterInt>    (ParameterID { id::distance, 1 }, "Distance", -24, 24, 12, signedText),
        std::make_unique<AudioParameterInt>    (ParameterID { id::steps, 1 }, "Steps", 0, 8, 0),

        std::make_unique<AudioParameterBool>   (ParameterID { id::velocityOn, 1 }, "Velocity", false),
        std::make_unique<AudioParameterFloat>  (ParameterID { id::velocityDecay, 1 }, "Velocity Decay",
                                                skewedRange (0.0f, 10000.0f, 1000.0f, 1.0f), 1000.0f, msText()),
        std::make_unique<AudioParameterInt>    (ParameterID { id::velocityTarget, 1 }, "Velocity Target", 0, 127, 0),
        std::make_unique<AudioParameterBool>   (ParameterID { id::velocityRetrig, 1 }, "Velocity Retrigger", true),
    };
}

} // namespace params
