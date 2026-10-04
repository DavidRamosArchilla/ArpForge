#include "PluginEditor.h"
#include "Parameters.h"
#include "engine/Scales.h"

using namespace theme;

namespace
{
    juce::RangedAudioParameter& param (APVTS& state, const char* id)
    {
        auto* p = state.getParameter (id);
        jassert (p != nullptr);
        return *p;
    }

    juce::StringArray choicesOf (APVTS& state, const char* id)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
            return p->choices;
        return {};
    }

    float valueOf (APVTS& state, const char* id)
    {
        return state.getRawParameterValue (id)->load();
    }

    void dim (juce::Component& c, bool inUse)
    {
        const float alpha = inUse ? 1.0f : 0.35f;
        if (c.getAlpha() != alpha)
            c.setAlpha (alpha);
    }
} // namespace

//==============================================================================
ArpForgeEditor::Content::Content (ArpForgeProcessor& p)
    : processor (p),
      state (p.state),
      style (param (state, params::id::style), params::styleNames()),
      hold (state, params::id::hold, "HOLD"),
      sync (state, params::id::sync, "SYNC"),
      velocityOn (state, params::id::velocityOn, "ON"),
      velocityRetrig (state, params::id::velocityRetrig, "RETRIG"),
      groove (param (state, params::id::groove), choicesOf (state, params::id::groove)),
      retrigger (param (state, params::id::retrigger), choicesOf (state, params::id::retrigger)),
      transposeMode (param (state, params::id::transposeMode), choicesOf (state, params::id::transposeMode)),
      rate (state, params::id::rate, "RATE"),
      freeRate (state, params::id::freeRate, "RATE"),
      gate (state, params::id::gate, "GATE"),
      swing (state, params::id::swing, "SWING"),
      offset (state, params::id::offset, "OFFSET"),
      repeats (state, params::id::repeats, "REPEATS"),
      retriggerRate (state, params::id::retriggerRate, "EVERY"),
      distance (state, params::id::distance, "DISTANCE", true),
      steps (state, params::id::steps, "STEPS"),
      velocityDecay (state, params::id::velocityDecay, "DECAY"),
      velocityTarget (state, params::id::velocityTarget, "TARGET"),
      key (state, params::id::transposeKey),
      scale (state, params::id::transposeScale)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &style, &pattern, &hold, &sync, &velocityOn, &velocityRetrig,
             &groove, &retrigger, &transposeMode,
             &rate, &freeRate, &gate, &swing, &offset, &repeats, &retriggerRate,
             &distance, &steps, &velocityDecay, &velocityTarget, &key, &scale })
        addAndMakeVisible (c);

    setSize (baseWidth, baseHeight);
    layout();
    refresh();
}

void ArpForgeEditor::Content::layout()
{
    panels = {{
        { "PATTERN",   { 16, 64, 500, 210 } },
        { "RHYTHM",    { 528, 64, 276, 210 } },
        { "SEQUENCE",  { 16, 286, 236, 198 } },
        { "TRANSPOSE", { 264, 286, 296, 198 } },
        { "VELOCITY",  { 572, 286, 232, 198 } },
    }};

    hold.setBounds (baseWidth - 20 - 84, 15, 84, 28);

    // Inside a panel: a row of small controls under the title, then knobs.
    auto topRow = [] (const Panel& p) { return juce::Rectangle<int> (p.bounds.getX() + 16, p.bounds.getY() + 38, p.bounds.getWidth() - 32, 26); };
    auto knobRow = [] (const Panel& p) { return juce::Rectangle<int> (p.bounds.getX() + 16, p.bounds.getY() + 76, p.bounds.getWidth() - 32, 108); };

    auto placeKnobs = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> knobs)
    {
        const int w = row.getWidth() / (int) knobs.size();
        for (auto* k : knobs)
            k->setBounds (row.removeFromLeft (w));
    };

    // Pattern
    const auto& pp = panels[0].bounds;
    style.setBounds (pp.getX() + 16, pp.getY() + 36, pp.getWidth() - 32, 34);
    pattern.setBounds (pp.getX() + 16, pp.getY() + 80, pp.getWidth() - 32, pp.getHeight() - 96);

    // Rhythm
    {
        auto row = topRow (panels[1]);
        sync.setBounds (row.removeFromLeft (62));
        row.removeFromLeft (10);
        groove.setBounds (row);

        placeKnobs (knobRow (panels[1]), { &rate, &gate, &swing });
        freeRate.setBounds (rate.getBounds());
    }

    // Sequence
    retrigger.setBounds (topRow (panels[2]));
    placeKnobs (knobRow (panels[2]), { &offset, &repeats, &retriggerRate });

    // Transpose
    {
        auto row = topRow (panels[3]);
        transposeMode.setBounds (row.removeFromLeft (92));
        row.removeFromLeft (8);
        key.setBounds (row.removeFromLeft (54));
        row.removeFromLeft (8);
        scale.setBounds (row);

        placeKnobs (knobRow (panels[3]), { &distance, &steps });
    }

    // Velocity
    {
        auto row = topRow (panels[4]);
        velocityOn.setBounds (row.removeFromLeft (56));
        row.removeFromLeft (8);
        velocityRetrig.setBounds (row.removeFromLeft (76));

        placeKnobs (knobRow (panels[4]), { &velocityDecay, &velocityTarget });
    }
}

void ArpForgeEditor::Content::refresh()
{
    processor.copySnapshot (snapshot);
    pattern.update (snapshot);

    // Show the controls that matter for the current settings.
    const bool synced = valueOf (state, params::id::sync) >= 0.5f;
    rate.setVisible (synced);
    freeRate.setVisible (! synced);

    dim (groove, synced);
    dim (swing, synced && valueOf (state, params::id::groove) >= 0.5f);
    dim (retriggerRate, juce::roundToInt (valueOf (state, params::id::retrigger)) == 2);

    const bool keyMode = valueOf (state, params::id::transposeMode) >= 0.5f;
    dim (key, keyMode);
    dim (scale, keyMode);

    const bool velocity = valueOf (state, params::id::velocityOn) >= 0.5f;
    dim (velocityRetrig, velocity);
    dim (velocityDecay, velocity);
    dim (velocityTarget, velocity);

    // Header activity light.
    if (snapshot.stepCounter != lastStepCounter)
    {
        lastStepCounter = snapshot.stepCounter;
        activity = 1.0f;
    }
    else
    {
        activity *= 0.78f;
    }

    repaint (hold.getBounds().getX() - 30, 0, 30, 56);
}

void ArpForgeEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (colour::background);
    drawHeader (g);

    for (const auto& panel : panels)
    {
        const auto b = panel.bounds.toFloat();
        g.setColour (colour::panel);
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (colour::panelEdge);
        g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);

        g.setColour (colour::textFaint);
        g.setFont (caps (10.0f));
        g.drawText (panel.title, panel.bounds.getX() + 16, panel.bounds.getY() + 12, 200, 14,
                    juce::Justification::centredLeft, false);
    }
}

void ArpForgeEditor::Content::drawHeader (juce::Graphics& g)
{
    // Logo: an ember tile with three rising steps.
    const auto tile = juce::Rectangle<float> (20.0f, 16.0f, 26.0f, 26.0f);
    g.setGradientFill (juce::ColourGradient (colour::accentHot, tile.getTopLeft(), colour::accent, tile.getBottomRight(), false));
    g.fillRoundedRectangle (tile, 7.0f);

    g.setColour (colour::background);
    for (int i = 0; i < 3; ++i)
    {
        const float h = 5.0f + 4.0f * (float) i;
        g.fillRoundedRectangle (tile.getX() + 6.0f + 5.5f * (float) i, tile.getBottom() - 6.0f - h, 3.5f, h, 1.5f);
    }

    // Wordmark.
    const auto titleFont = font (21.0f, Weight::semibold);
    const float arpWidth = juce::GlyphArrangement::getStringWidth (titleFont, "Arp");
    const float forgeWidth = juce::GlyphArrangement::getStringWidth (titleFont, "Forge");
    g.setFont (titleFont);
    g.setColour (colour::text);
    g.drawText ("Arp", juce::Rectangle<float> (56.0f, 14.0f, arpWidth + 2.0f, 30.0f), juce::Justification::centredLeft, false);
    g.setColour (colour::accent);
    g.drawText ("Forge", juce::Rectangle<float> (56.0f + arpWidth, 14.0f, forgeWidth + 2.0f, 30.0f), juce::Justification::centredLeft, false);

    g.setColour (colour::textFaint);
    g.setFont (caps (10.0f));
    g.drawText ("ARPEGGIATOR", juce::Rectangle<float> (66.0f + arpWidth + forgeWidth, 14.0f, 120.0f, 31.0f),
                juce::Justification::centredLeft, false);

    // Activity light, next to Hold.
    const auto led = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ (float) hold.getX() - 16.0f, (float) hold.getBounds().getCentreY() });
    if (activity > 0.02f)
    {
        g.setColour (colour::accent.withAlpha (0.3f * activity));
        g.fillEllipse (led.expanded (4.0f));
    }
    g.setColour (colour::idleNote.interpolatedWith (colour::accent, activity));
    g.fillEllipse (led);
}

//==============================================================================
ArpForgeEditor::ArpForgeEditor (ArpForgeProcessor& p)
    : AudioProcessorEditor (p),
      content (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

ArpForgeEditor::~ArpForgeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ArpForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (colour::background);
}

void ArpForgeEditor::resized()
{
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) baseWidth));
}

void ArpForgeEditor::timerCallback()
{
    content.refresh();
}
