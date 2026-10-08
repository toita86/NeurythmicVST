#include "../include/Neurythmic/IconButton.h"

#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

IconButton::IconButton(Icon icon) : _icon(icon) {
  setRepaintsOnMouseActivity(true);
}

void IconButton::paint(juce::Graphics& g) {
  g.fillAll(_hovered ? MenuTheme::hoverBackground : MenuTheme::background);
  g.setColour(MenuTheme::labelColour);

  const auto b = getLocalBounds().reduced(7).toFloat();
  if (_icon == Icon::Hamburger) {
    const float cy = b.getCentreY();
    const float spacing = b.getHeight() * 0.22f;
    for (int i = -1; i <= 1; ++i)
      g.drawLine(b.getX(), cy + i * spacing, b.getRight(), cy + i * spacing,
                 1.5f);
  } else {
    g.drawLine(b.getX(), b.getY(), b.getRight(), b.getBottom(), 1.5f);
    g.drawLine(b.getRight(), b.getY(), b.getX(), b.getBottom(), 1.5f);
  }
}

void IconButton::mouseUp(const juce::MouseEvent& e) {
  if (contains(e.position.toInt()) && onClick)
    onClick();
}

void IconButton::mouseEnter(const juce::MouseEvent&) {
  _hovered = true;
  repaint();
}

void IconButton::mouseExit(const juce::MouseEvent&) {
  _hovered = false;
  repaint();
}

}  // namespace neurythmic
