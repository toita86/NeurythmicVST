#pragma once

#include <juce_graphics/juce_graphics.h>

/*
 * Dark theme for the Phase 5 menus, ported from the legacy
 * ofxDatGuiThemeMatsuoka (menu_themes.h). Kept as a single palette so all
 * widgets and menus share the same look.
 */

namespace neurythmic::MenuTheme {
inline const juce::Colour guiBackground{0xff2c3137};
inline const juce::Colour background{0xff343b41};
inline const juce::Colour labelColour{0xfff8f3f0};
inline const juce::Colour sliderFill{0xff6e6e6e};
inline const juce::Colour sliderText{0xffffffff};
inline const juce::Colour inputAreaBackground{0xff434a50};
inline const juce::Colour hoverBackground{0xff434a50};
inline const juce::Colour downBackground{0xff2c3137};
inline const juce::Colour matrixHoverButton{0xff60b9ed};
inline const juce::Colour matrixSelectedButton{0xff2c3137};
inline const juce::Colour warningBackground{0xff943137};
inline const juce::Colour blueBackground{0xff213e6f};
}  // namespace neurythmic::MenuTheme
