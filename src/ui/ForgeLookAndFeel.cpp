#include "ForgeLookAndFeel.h"

using namespace theme;

ForgeLookAndFeel::ForgeLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colour::background);
    setColour (juce::ComboBox::textColourId, colour::text);
    setColour (juce::ComboBox::backgroundColourId, colour::track);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::arrowColourId, colour::textDim);
    setColour (juce::PopupMenu::backgroundColourId, colour::panel);
    setColour (juce::PopupMenu::textColourId, colour::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::accent.withAlpha (0.16f));
    setColour (juce::PopupMenu::highlightedTextColourId, colour::text);
    setColour (juce::TextButton::buttonColourId, colour::track);
    setColour (juce::Slider::textBoxTextColourId, colour::text);
}

void ForgeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float stroke = 3.5f;
    const float radius = diameter * 0.5f - stroke;
    const bool hover = slider.isMouseOverOrDragging();

    const juce::PathStrokeType arcStroke (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (hover ? colour::trackHover : colour::track);
    g.strokePath (track, arcStroke);

    const bool bipolar = slider.getProperties()["bipolar"];
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                             juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (colour::accent);
        g.strokePath (value, arcStroke);
    }

    // Inner disc with the current value.
    const float discRadius = radius - stroke - 3.0f;
    g.setColour (colour::well);
    g.fillEllipse (juce::Rectangle<float> (discRadius * 2.0f, discRadius * 2.0f).withCentre (centre));
    g.setColour (hover ? colour::trackHover : colour::panelEdge);
    g.drawEllipse (juce::Rectangle<float> (discRadius * 2.0f, discRadius * 2.0f).withCentre (centre), 1.0f);

    const auto tip = centre.getPointOnCircumference (radius, angle);
    g.setColour (colour::text);
    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (tip));

    g.setColour (colour::text);
    g.setFont (font (juce::jlimit (10.0f, 13.0f, diameter * 0.19f), Weight::medium));
    g.drawFittedText (slider.getTextFromValue (slider.getValue()),
                      juce::Rectangle<float> (discRadius * 1.8f, discRadius * 1.2f).withCentre (centre).toNearestInt(),
                      juce::Justification::centred, 1, 0.8f);
}

void ForgeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                             bool isHighlighted, bool isDown)
{
    const auto b = button.getLocalBounds().toFloat().reduced (0.5f);
    const float corner = b.getHeight() * 0.5f;

    if (button.getToggleState())
    {
        g.setColour (colour::accent.withAlpha (isDown ? 0.28f : (isHighlighted ? 0.22f : 0.16f)));
        g.fillRoundedRectangle (b, corner);
        g.setColour (colour::accent);
        g.drawRoundedRectangle (b, corner, 1.0f);
    }
    else
    {
        g.setColour (isDown ? colour::trackHover.brighter (0.1f) : (isHighlighted ? colour::trackHover : colour::track));
        g.fillRoundedRectangle (b, corner);
    }
}

void ForgeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool isHighlighted, bool)
{
    const bool on = button.getToggleState();
    g.setColour (on ? colour::accentHot : (isHighlighted ? colour::text : colour::textDim));
    g.setFont (caps (10.5f));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

void ForgeLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                                     juce::ComboBox& box)
{
    const auto b = juce::Rectangle<float> ((float) width, (float) height).reduced (0.5f);
    g.setColour (box.isMouseOver (true) ? colour::trackHover : colour::track);
    g.fillRoundedRectangle (b, 6.0f);

    const float cx = (float) width - 12.0f;
    const float cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 3.5f, cy - 1.5f);
    chevron.lineTo (cx, cy + 2.0f);
    chevron.lineTo (cx + 3.5f, cy - 1.5f);
    g.setColour (colour::textDim);
    g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font ForgeLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return font (12.5f, Weight::medium);
}

void ForgeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 0, box.getWidth() - 22, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setMinimumHorizontalScale (0.75f);
}

void ForgeLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (colour::panel);
    g.setColour (colour::panelEdge);
    g.drawRect (0, 0, width, height, 1);
}

void ForgeLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator,
                                          bool isActive, bool isHighlighted, bool isTicked, bool,
                                          const juce::String& text, const juce::String&, const juce::Drawable*,
                                          const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (colour::panelEdge);
        g.fillRect (area.reduced (10, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    auto r = area.reduced (3, 1);

    if (isHighlighted && isActive)
    {
        g.setColour (colour::accent.withAlpha (0.16f));
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
    }

    if (isTicked)
    {
        g.setColour (colour::accent);
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ (float) r.getX() + 11.0f, (float) r.getCentreY() }));
    }

    g.setColour (! isActive ? colour::textFaint : (isTicked ? colour::accentHot : colour::text));
    g.setFont (getPopupMenuFont());
    g.drawText (text, r.withTrimmedLeft (22).withTrimmedRight (8), juce::Justification::centredLeft, true);
}

juce::Font ForgeLookAndFeel::getPopupMenuFont()
{
    return font (13.5f, Weight::medium);
}

void ForgeLookAndFeel::getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int,
                                                  int& idealWidth, int& idealHeight)
{
    idealHeight = isSeparator ? 9 : 26;
    idealWidth = isSeparator ? 50 : juce::roundToInt (juce::GlyphArrangement::getStringWidth (getPopupMenuFont(), text)) + 44;
}

juce::Typeface::Ptr ForgeLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
        return faces->regular;

    return LookAndFeel_V4::getTypefaceForFont (f);
}
