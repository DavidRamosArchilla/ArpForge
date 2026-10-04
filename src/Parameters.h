#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace params
{

namespace id
{
    inline constexpr const char* style            = "style";
    inline constexpr const char* sync             = "sync";
    inline constexpr const char* rate             = "rate";
    inline constexpr const char* freeRate         = "freeRate";
    inline constexpr const char* gate             = "gate";
    inline constexpr const char* groove           = "groove";
    inline constexpr const char* swing            = "swing";
    inline constexpr const char* hold             = "hold";
    inline constexpr const char* offset           = "offset";
    inline constexpr const char* repeats          = "repeats";
    inline constexpr const char* retrigger        = "retrigger";
    inline constexpr const char* retriggerRate    = "retriggerRate";
    inline constexpr const char* transposeMode    = "transposeMode";
    inline constexpr const char* transposeKey     = "transposeKey";
    inline constexpr const char* transposeScale   = "transposeScale";
    inline constexpr const char* distance         = "distance";
    inline constexpr const char* steps            = "steps";
    inline constexpr const char* velocityOn       = "velocityOn";
    inline constexpr const char* velocityDecay    = "velocityDecay";
    inline constexpr const char* velocityTarget   = "velocityTarget";
    inline constexpr const char* velocityRetrig   = "velocityRetrig";
} // namespace id

struct NamedLength
{
    const char* name;
    double beats;
};

inline const juce::StringArray& styleNames()
{
    static const juce::StringArray names {
        "Up", "Down", "UpDown", "DownUp", "Up & Down", "Down & Up",
        "Converge", "Diverge", "Con & Diverge",
        "Pinky Up", "Pinky UpDown", "Thumb Up", "Thumb UpDown",
        "Play Order", "Chord Trigger", "Random", "Random Other", "Random Once"
    };
    return names;
}

inline const std::array<NamedLength, 16>& rates()
{
    static const std::array<NamedLength, 16> list {{
        { "1/1", 4.0 },       { "1/2D", 3.0 },     { "1/2", 2.0 },     { "1/2T", 4.0 / 3.0 },
        { "1/4D", 1.5 },      { "1/4", 1.0 },      { "1/4T", 2.0 / 3.0 },
        { "1/8D", 0.75 },     { "1/8", 0.5 },      { "1/8T", 1.0 / 3.0 },
        { "1/16D", 0.375 },   { "1/16", 0.25 },    { "1/16T", 1.0 / 6.0 },
        { "1/32", 0.125 },    { "1/32T", 1.0 / 12.0 }, { "1/64", 0.0625 },
    }};
    return list;
}

inline constexpr int defaultRateIndex = 11; // 1/16

inline const std::array<NamedLength, 8>& retriggerRates()
{
    static const std::array<NamedLength, 8> list {{
        { "1/16", 0.25 }, { "1/8", 0.5 }, { "1/4", 1.0 }, { "1/2", 2.0 },
        { "1 Bar", 4.0 }, { "2 Bars", 8.0 }, { "4 Bars", 16.0 }, { "8 Bars", 32.0 },
    }};
    return list;
}

inline constexpr int infiniteRepeats = 33;   // the top of the Repeats range means "forever"

inline const juce::StringArray& keyNames()
{
    static const juce::StringArray names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

} // namespace params
