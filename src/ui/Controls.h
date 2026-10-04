#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

using APVTS = juce::AudioProcessorValueTreeState;

/** Rotary knob with its value inside and a caption underneath. */
class Knob final : public juce::Component
{
public:
    Knob (APVTS& state, const juce::String& paramId, const juce::String& caption, bool bipolar = false);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider slider;
    juce::String caption;
    APVTS::SliderAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};

/** Pill-shaped on/off button bound to a bool parameter. */
class Toggle final : public juce::TextButton
{
public:
    Toggle (APVTS& state, const juce::String& paramId, const juce::String& text);

private:
    APVTS::ButtonAttachment attachment;
};

/** Row of mutually exclusive segments bound to a choice parameter. */
class Segmented final : public juce::Component
{
public:
    Segmented (juce::RangedAudioParameter& param, juce::StringArray labels);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    int segmentAt (juce::Point<int> p) const;

    juce::StringArray labels;
    int selected = 0, hovered = -1;
    juce::ParameterAttachment attachment;
};

/** Combo box bound to a choice parameter. */
class Choice final : public juce::ComboBox
{
public:
    Choice (APVTS& state, const juce::String& paramId);

private:
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

/** A list picker: arrows step through the items, clicking the name opens a
    menu with one column per section. */
class Selector final : public juce::Component
{
public:
    struct Section
    {
        juce::String title;
        int first, last;
        std::vector<int> separatorsAfter;
    };

    Selector (juce::StringArray names, std::vector<Section> sections, float textHeight, bool showCount);

    /** Called when the user picks an item. */
    std::function<void (int)> onSelect;

    /** Updates what's shown, without calling onSelect. */
    void setSelected (int index);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    enum class Zone { none, previous, name, next };
    Zone zoneAt (juce::Point<int> p) const;
    void pick (int index);
    void showMenu();

    juce::StringArray names;
    std::vector<Section> sections;
    float textHeight;
    bool showCount;
    int selected = 0;
    Zone hovered = Zone::none;
};
