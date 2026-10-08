#include "../include/Neurythmic/ConstraintGUI.h"

#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

ConstraintGUI::ConstraintGUI() = default;

void ConstraintGUI::setGrid(MatsuokaEngine::gridType grid) {
  switch (grid) {
    case MatsuokaEngine::gridType::_24th:
      _gridlineCount = 24;
      break;
    case MatsuokaEngine::gridType::_32nd:
      _gridlineCount = 32;
      break;
    case MatsuokaEngine::gridType::unQuantised:
    default:
      _gridlineCount = 0;
      break;
  }
  repaint();
}

void ConstraintGUI::setGridMultiplier(int mult) {
  _multiple = mult;
  repaint();
}

void ConstraintGUI::setGridOffset(int off) {
  _offset = off;
  repaint();
}

void ConstraintGUI::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::guiBackground);
  if (_gridlineCount <= 0)
    return;

  const float w = static_cast<float>(getWidth());
  const float h = static_cast<float>(getHeight());
  g.setColour(MenuTheme::sliderFill);
  for (int i = 0; i <= _gridlineCount; ++i) {
    const float x = static_cast<float>(i) * w / _gridlineCount;
    g.drawLine(x, 0.0f, x, h, 1.0f);
  }

  if (_offset > 0 && _offset <= _gridlineCount) {
    const float x = static_cast<float>(_offset) * w / _gridlineCount;
    g.setColour(MenuTheme::matrixHoverButton);
    g.drawLine(x, 0.0f, x, h, 2.0f);
  }
}

}  // namespace neurythmic
