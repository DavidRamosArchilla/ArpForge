#pragma once

#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/ForgeLookAndFeel.h"
#include "ui/MidiDragTile.h"
#include "ui/PatternView.h"

class ArpForgeEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ArpForgeEditor (ArpForgeProcessor&);
    ~ArpForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int baseWidth = 820;
    static constexpr int baseHeight = 500;

private:
    /** Everything is laid out at a fixed size here and scaled as a whole. */
    class Content final : public juce::Component
    {
    public:
        explicit Content (ArpForgeProcessor&);
        void paint (juce::Graphics&) override;
        void refresh();

    private:
        struct Panel
        {
            juce::String title;
            juce::Rectangle<int> bounds;
        };

        void drawHeader (juce::Graphics&);
        void layout();

        ArpForgeProcessor& processor;
        APVTS& state;

        Selector style, presets;
        juce::ParameterAttachment styleAttachment;
        PatternView pattern;
        MidiDragTile midiTile;
        Toggle hold, sync, velocityOn, velocityRetrig;
        Segmented groove, retrigger, transposeMode;
        Knob rate, freeRate, gate, swing;
        Knob offset, repeats, retriggerRate;
        Knob distance, steps;
        Knob velocityDecay, velocityTarget;
        Choice key, scale;

        std::array<Panel, 5> panels;
        arp::Snapshot snapshot;
        uint32_t lastStepCounter = 0;
        float activity = 0.0f;
    };

    void timerCallback() override;

    ForgeLookAndFeel lookAndFeel;
    Content content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpForgeEditor)
};
