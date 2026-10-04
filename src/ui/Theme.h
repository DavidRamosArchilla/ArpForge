#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace theme
{
namespace colour
{
    inline const juce::Colour background   { 0xff0d0e11 };
    inline const juce::Colour panel        { 0xff15171c };
    inline const juce::Colour panelEdge    { 0xff22252c };
    inline const juce::Colour well         { 0xff0f1014 };
    inline const juce::Colour track        { 0xff2a2e37 };
    inline const juce::Colour trackHover   { 0xff363b46 };
    inline const juce::Colour idleNote     { 0xff3b414d };
    inline const juce::Colour text         { 0xffe9ebef };
    inline const juce::Colour textDim      { 0xff8b919c };
    inline const juce::Colour textFaint    { 0xff545a66 };
    inline const juce::Colour accent       { 0xffff7a2f };
    inline const juce::Colour accentHot    { 0xffffb35c };
} // namespace colour

enum class Weight { regular, medium, semibold };

/** The embedded Inter faces. Shared through juce::SharedResourcePointer. */
struct Typefaces
{
    Typefaces();
    juce::Typeface::Ptr regular, medium, semibold;
};

juce::Font font (float height, Weight weight = Weight::regular);

/** Small uppercase label text with a little letter spacing. */
juce::Font caps (float height = 10.5f);

} // namespace theme
