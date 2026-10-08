#include "../include/Neurythmic/RadioButtonMatrix.h"

#include <cmath>

#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

RadioButtonMatrix::RadioButtonMatrix(const juce::StringArray& labels,
                                     int columns,
                                     Listener onChange)
    : _labels(labels), _columns(columns), _onChange(std::move(onChange)) {}

void RadioButtonMatrix::setSelected(int index) {
  if (index < 0 || index >= _labels.size())
    return;
  if (_selected != index) {
    _selected = index;
    repaint();
  }
}

void RadioButtonMatrix::setLabels(const juce::StringArray& labels) {
  _labels = labels;
  if (_selected >= _labels.size())
    _selected = _labels.size() - 1;
  repaint();
}

juce::Rectangle<int> RadioButtonMatrix::cellBounds(int index) const {
  const int rows = static_cast<int>(
      std::ceil(_labels.size() / static_cast<double>(_columns)));
  const int cellW = getWidth() / _columns;
  const int cellH = getHeight() / rows;
  const int col = index % _columns;
  const int row = index / _columns;
  return juce::Rectangle<int>(col * cellW + 2, row * cellH + 2, cellW - 4,
                              cellH - 4);
}

int RadioButtonMatrix::indexAt(juce::Point<int> pos) const {
  for (int i = 0; i < _labels.size(); ++i)
    if (cellBounds(i).contains(pos))
      return i;
  return -1;
}

void RadioButtonMatrix::resized() {
  repaint();
}

void RadioButtonMatrix::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::guiBackground);
  for (int i = 0; i < _labels.size(); ++i) {
    const auto r = cellBounds(i).toFloat();
    const bool sel = i == _selected;
    const bool hover = isMouseOver() && indexAt(getMouseXYRelative()) == i;

    g.setColour(sel     ? MenuTheme::matrixSelectedButton
                : hover ? MenuTheme::matrixHoverButton
                        : MenuTheme::background);
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(sel ? MenuTheme::matrixHoverButton : MenuTheme::labelColour);
    g.setFont(10.0f);
    g.drawText(_labels[i], r, juce::Justification::centred);
  }
}

void RadioButtonMatrix::mouseDown(const juce::MouseEvent& e) {
  const int idx = indexAt(e.position.toInt());
  if (idx < 0)
    return;
  setSelected(idx);
  if (_onChange)
    _onChange(idx);
}

void RadioButtonMatrix::mouseMove(const juce::MouseEvent&) {
  repaint();
}

void RadioButtonMatrix::mouseExit(const juce::MouseEvent&) {
  repaint();
}

}  // namespace neurythmic
