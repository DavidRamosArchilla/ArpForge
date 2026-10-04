#include "Controls.h"

using namespace theme;

//==============================================================================
Knob::Knob (APVTS& state, const juce::String& paramId, const juce::String& captionText, bool bipolar)
    : slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox),
      caption (captionText),
      attachment (state, paramId, slider)
{
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    slider.setScrollWheelEnabled (true);
    slider.setRepaintsOnMouseActivity (true);
    slider.getProperties().set ("bipolar", bipolar);

    if (auto* param = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    addAndMakeVisible (slider);
}

void Knob::resized()
{
    auto area = getLocalBounds();
    area.removeFromBottom (18);
    const int size = juce::jmin (area.getWidth(), area.getHeight(), 66);
    slider.setBounds (area.withSizeKeepingCentre (size, size));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (colour::textDim);
    g.setFont (caps (10.0f));
    g.drawText (caption, getLocalBounds().removeFromBottom (16), juce::Justification::centred, false);
}

//==============================================================================
Toggle::Toggle (APVTS& state, const juce::String& paramId, const juce::String& text)
    : juce::TextButton (text),
      attachment (state, paramId, *this)
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

//==============================================================================
Segmented::Segmented (juce::RangedAudioParameter& param, juce::StringArray segmentLabels)
    : labels (std::move (segmentLabels)),
      attachment (param, [this] (float v)
                  {
                      selected = juce::jlimit (0, labels.size() - 1, juce::roundToInt (v));
                      repaint();
                  })
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

int Segmented::segmentAt (juce::Point<int> p) const
{
    if (! getLocalBounds().contains (p) || labels.isEmpty())
        return -1;

    return juce::jlimit (0, labels.size() - 1, p.x * labels.size() / juce::jmax (1, getWidth()));
}

void Segmented::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = b.getHeight() * 0.5f;

    g.setColour (colour::track);
    g.fillRoundedRectangle (b, corner);

    const float w = b.getWidth() / (float) labels.size();
    g.setFont (font (12.0f, Weight::medium));

    for (int i = 0; i < labels.size(); ++i)
    {
        const auto seg = juce::Rectangle<float> (b.getX() + w * (float) i, b.getY(), w, b.getHeight());

        if (i == selected)
        {
            const auto pill = seg.reduced (2.0f);
            g.setColour (colour::accent.withAlpha (0.18f));
            g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);
            g.setColour (colour::accent);
            g.drawRoundedRectangle (pill, pill.getHeight() * 0.5f, 1.0f);
            g.setColour (colour::accentHot);
        }
        else
        {
            g.setColour (i == hovered ? colour::text : colour::textDim);
        }

        g.drawText (labels[i], seg, juce::Justification::centred, false);
    }
}

void Segmented::mouseDown (const juce::MouseEvent& e)
{
    const int index = segmentAt (e.getPosition());
    if (index >= 0 && index != selected)
        attachment.setValueAsCompleteGesture ((float) index);
}

void Segmented::mouseMove (const juce::MouseEvent& e)
{
    const int index = segmentAt (e.getPosition());
    if (index != hovered)
    {
        hovered = index;
        repaint();
    }
}

void Segmented::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

//==============================================================================
Choice::Choice (APVTS& state, const juce::String& paramId)
{
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId)))
        addItemList (param->choices, 1);

    attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, *this);
    setRepaintsOnMouseActivity (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

//==============================================================================
Selector::Selector (juce::StringArray itemNames, std::vector<Section> menuSections, float nameHeight, bool withCount)
    : names (std::move (itemNames)),
      sections (std::move (menuSections)),
      textHeight (nameHeight),
      showCount (withCount)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Selector::setSelected (int index)
{
    index = juce::jlimit (0, juce::jmax (0, names.size() - 1), index);
    if (index != selected)
    {
        selected = index;
        repaint();
    }
}

Selector::Zone Selector::zoneAt (juce::Point<int> p) const
{
    if (! getLocalBounds().contains (p))
        return Zone::none;

    const int arrowWidth = getHeight();
    if (p.x < arrowWidth)
        return Zone::previous;
    if (p.x >= getWidth() - arrowWidth)
        return Zone::next;
    return Zone::name;
}

void Selector::pick (int index)
{
    const int n = names.size();
    index = ((index % n) + n) % n;
    setSelected (index);

    if (onSelect)
        onSelect (index);
}

void Selector::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float corner = juce::jmin (8.0f, b.getHeight() * 0.5f);
    g.setColour (hovered == Zone::name ? colour::trackHover : colour::track);
    g.fillRoundedRectangle (b, corner);

    const float arrowWidth = b.getHeight();

    auto drawChevron = [&] (juce::Rectangle<float> area, bool pointsLeft, bool hot)
    {
        if (hot)
        {
            g.setColour (colour::trackHover.brighter (0.08f));
            g.fillRoundedRectangle (area.reduced (3.0f), corner - 2.0f);
        }

        const auto c = area.getCentre();
        const float s = juce::jmin (4.5f, area.getHeight() * 0.15f), dir = pointsLeft ? 1.0f : -1.0f;
        juce::Path p;
        p.startNewSubPath (c.x + s * 0.5f * dir, c.y - s);
        p.lineTo (c.x - s * 0.5f * dir, c.y);
        p.lineTo (c.x + s * 0.5f * dir, c.y + s);
        g.setColour (hot ? colour::accentHot : colour::textDim);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    drawChevron (b.withWidth (arrowWidth), true, hovered == Zone::previous);
    drawChevron (b.withLeft (b.getRight() - arrowWidth), false, hovered == Zone::next);

    g.setColour (colour::text);
    g.setFont (font (textHeight, Weight::semibold));
    g.drawFittedText (names[selected], b.reduced (arrowWidth, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);

    if (showCount)
    {
        g.setColour (colour::textFaint);
        g.setFont (font (11.0f, Weight::medium));
        g.drawText (juce::String (selected + 1) + " / " + juce::String (names.size()),
                    b.reduced (arrowWidth + 6.0f, 0.0f), juce::Justification::centredRight, false);
    }
}

void Selector::showMenu()
{
    juce::PopupMenu menu;

    for (size_t s = 0; s < sections.size(); ++s)
    {
        const auto& section = sections[s];
        if (s > 0)
            menu.addColumnBreak();

        menu.addSectionHeader (section.title);

        for (int i = section.first; i <= section.last && i < names.size(); ++i)
        {
            menu.addItem (i + 1, names[i], true, i == selected);
            if (std::find (section.separatorsAfter.begin(), section.separatorsAfter.end(), i) != section.separatorsAfter.end())
                menu.addSeparator();
        }
    }

    juce::Component::SafePointer<Selector> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this)
                                                  .withStandardItemHeight (26),
                        [safeThis] (int result)
                        {
                            if (safeThis != nullptr && result > 0)
                                safeThis->pick (result - 1);
                        });
}

void Selector::mouseDown (const juce::MouseEvent& e)
{
    switch (zoneAt (e.getPosition()))
    {
        case Zone::previous: pick (selected - 1); break;
        case Zone::next:     pick (selected + 1); break;
        case Zone::name:     showMenu(); break;
        case Zone::none:     break;
    }
}

void Selector::mouseMove (const juce::MouseEvent& e)
{
    const auto zone = zoneAt (e.getPosition());
    if (zone != hovered)
    {
        hovered = zone;
        repaint();
    }
}

void Selector::mouseExit (const juce::MouseEvent&)
{
    hovered = Zone::none;
    repaint();
}
