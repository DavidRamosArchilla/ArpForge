#pragma once

#include "Theme.h"
#include "../engine/ArpTypes.h"

/** Mini piano roll of the current arpeggio: one lane per held note, one
    column per step, with the step being played lit up. */
class PatternView final : public juce::Component
{
public:
    /** Called by the editor's timer with the latest engine snapshot. */
    void update (const arp::Snapshot& snapshot);

    void paint (juce::Graphics&) override;

private:
    arp::Snapshot snap;
    std::array<float, arp::Snapshot::maxSteps> glow {};
    uint32_t lastCounter = 0;
    int litTranspose = 0;
};
