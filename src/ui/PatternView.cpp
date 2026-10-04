#include "PatternView.h"

using namespace theme;

namespace
{
    juce::String noteName (int note)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        // FL Studio numbering: note 60 is C5.
        return juce::String (names[note % 12]) + juce::String (note / 12);
    }
} // namespace

void PatternView::update (const arp::Snapshot& snapshot)
{
    for (auto& g : glow)
        g *= 0.8f;

    if (snapshot.stepCounter != lastCounter)
    {
        lastCounter = snapshot.stepCounter;

        if (snapshot.currentStep >= 0 && snapshot.currentStep < arp::Snapshot::maxSteps)
            glow[(size_t) snapshot.currentStep] = 1.0f;

        // The engine reports the transposition of the *next* step; the lit
        // one belongs to the step that just played.
        const int count = snapshot.transposeSteps + 1;
        const bool wrapped = snapshot.currentStep == snapshot.numSteps - 1;
        litTranspose = wrapped ? (snapshot.transposeIndex - 1 + count) % count : snapshot.transposeIndex;
    }

    if (! snapshot.active)
        std::fill (glow.begin(), glow.end(), 0.0f);

    snap = snapshot;
    repaint();
}

void PatternView::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    g.setColour (colour::well);
    g.fillRoundedRectangle (area, 8.0f);
    g.setColour (colour::panelEdge);
    g.drawRoundedRectangle (area.reduced (0.5f), 8.0f, 1.0f);

    if (snap.numSteps == 0)
    {
        g.setColour (colour::textFaint);
        g.setFont (font (12.5f, Weight::medium));
        g.drawText ("Play some notes to start the arpeggio", area, juce::Justification::centred, false);
        return;
    }

    // One lane per distinct pitch, lowest at the bottom.
    std::vector<int> pitches;
    for (int i = 0; i < snap.numSteps; ++i)
        for (int k = 0; k < snap.steps[(size_t) i].count; ++k)
            pitches.push_back (snap.steps[(size_t) i].notes[(size_t) k]);

    std::sort (pitches.begin(), pitches.end());
    pitches.erase (std::unique (pitches.begin(), pitches.end()), pitches.end());

    const int lanes = (int) pitches.size();
    auto inner = area.reduced (12.0f, 10.0f);
    const auto gutter = inner.removeFromLeft (34.0f);
    inner.removeFromRight (snap.transposeSteps > 0 ? 18.0f : 0.0f);

    const float laneHeight = inner.getHeight() / (float) lanes;
    const float columnWidth = juce::jmin (inner.getWidth() / (float) snap.numSteps, 44.0f);
    const float noteHeight = juce::jlimit (3.0f, 10.0f, laneHeight * 0.62f);

    auto laneY = [&] (int pitch)
    {
        const auto lane = (int) (std::lower_bound (pitches.begin(), pitches.end(), pitch) - pitches.begin());
        return inner.getBottom() - laneHeight * ((float) lane + 0.5f);
    };

    // Lanes and note names.
    g.setFont (font (10.0f, Weight::medium));
    const bool roomForNames = laneHeight >= 10.0f;

    for (int i = 0; i < lanes; ++i)
    {
        const float y = laneY (pitches[(size_t) i]);
        g.setColour (colour::panelEdge.withAlpha (0.6f));
        g.drawHorizontalLine (juce::roundToInt (y), inner.getX(), inner.getX() + columnWidth * (float) snap.numSteps);

        if (roomForNames || i == 0 || i == lanes - 1)
        {
            g.setColour (colour::textFaint);
            g.drawText (noteName (pitches[(size_t) i]), gutter.withY (y - 7.0f).withHeight (14.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    // Playhead column.
    if (snap.active && snap.currentStep >= 0)
    {
        g.setColour (colour::accent.withAlpha (0.07f));
        g.fillRoundedRectangle (inner.getX() + columnWidth * (float) snap.currentStep, inner.getY(),
                                columnWidth, inner.getHeight(), 4.0f);
    }

    // Notes.
    for (int i = 0; i < snap.numSteps; ++i)
    {
        const auto& step = snap.steps[(size_t) i];
        const float x = inner.getX() + columnWidth * (float) i;
        const float heat = glow[(size_t) i];
        const bool current = snap.active && i == snap.currentStep;

        for (int k = 0; k < step.count; ++k)
        {
            const auto note = juce::Rectangle<float> (x + 3.0f, laneY (step.notes[(size_t) k]) - noteHeight * 0.5f,
                                                      columnWidth - 6.0f, noteHeight);

            if (heat > 0.02f)
            {
                g.setColour (colour::accent.withAlpha (0.22f * heat));
                g.fillRoundedRectangle (note.expanded (3.0f), noteHeight * 0.5f + 3.0f);
            }

            g.setColour (current ? colour::accent : colour::idleNote.interpolatedWith (colour::accent, heat * 0.85f));
            g.fillRoundedRectangle (note, noteHeight * 0.5f);
        }
    }

    // Transposition steps: one dot per pass, the current one lit.
    if (snap.transposeSteps > 0)
    {
        const int count = snap.transposeSteps + 1;
        const float dot = 5.0f, gap = 4.0f;
        const float x = area.getRight() - 16.0f;
        float y = area.getCentreY() + (dot + gap) * (float) (count - 1) * 0.5f;

        for (int i = 0; i < count; ++i, y -= dot + gap)
        {
            const bool lit = snap.active && i == litTranspose;
            g.setColour (lit ? colour::accent : colour::idleNote);
            g.fillEllipse (juce::Rectangle<float> (dot, dot).withCentre ({ x, y }));
        }
    }
}
