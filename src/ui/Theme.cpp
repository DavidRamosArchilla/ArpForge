#include "Theme.h"
#include "BinaryData.h"

namespace theme
{

Typefaces::Typefaces()
    : regular  (juce::Typeface::createSystemTypefaceFor (BinaryData::InterRegular_ttf,  BinaryData::InterRegular_ttfSize)),
      medium   (juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf,   BinaryData::InterMedium_ttfSize)),
      semibold (juce::Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize))
{
}

juce::Font font (float height, Weight weight)
{
    // The editor's look-and-feel keeps the shared typefaces alive, so this
    // doesn't reload them; it only avoids static objects outliving JUCE.
    const juce::SharedResourcePointer<Typefaces> faces;

    const auto& face = weight == Weight::semibold ? faces->semibold
                     : weight == Weight::medium   ? faces->medium
                                                  : faces->regular;

    return juce::Font (juce::FontOptions (face).withHeight (height));
}

juce::Font caps (float height)
{
    return font (height, Weight::semibold).withExtraKerningFactor (0.12f);
}

} // namespace theme
