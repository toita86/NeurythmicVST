#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace neurythmic {

// Small clickable icon (hamburger or close) drawn with juce::Graphics, avoiding
// Unicode glyphs that may be missing from the button font.
class IconButton : public juce::Component {
public:
  enum class Icon { Hamburger, Close };

  explicit IconButton(Icon icon);
  ~IconButton() override = default;

  std::function<void()> onClick;

  void paint(juce::Graphics&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseEnter(const juce::MouseEvent&) override;
  void mouseExit(const juce::MouseEvent&) override;

private:
  Icon _icon;
  bool _hovered = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconButton)
};

}  // namespace neurythmic
