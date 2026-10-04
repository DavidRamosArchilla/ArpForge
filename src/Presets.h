#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace presets
{

struct Value
{
    const char* id;
    float value;   // in the parameter's own units: choice index, %, ms...
};

struct Preset
{
    const char* name;
    const char* category;
    std::vector<Value> values;
};

const std::vector<Preset>& all();

/** Resets every parameter except Hold to its default, then applies the preset. */
void apply (juce::AudioProcessorValueTreeState& state, const Preset& preset);

} // namespace presets
