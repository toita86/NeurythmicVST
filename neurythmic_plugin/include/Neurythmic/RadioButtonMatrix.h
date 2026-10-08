#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace neurythmic {

// A grid of labelled radio buttons (exactly one selected). Used as the basis
// for the frequency matrix, sync-mode matrix, grid-type matrix and the
// resolution selector.
class RadioButtonMatrix : public juce::Component {
public:
  using Listener = std::function<void(int)>;

  RadioButtonMatrix(const juce::StringArray& labels,
                    int columns,
                    Listener onChange = {});

  void setSelected(int index);
  int getSelectedIndex() const { return _selected; }
  void setLabels(const juce::StringArray& labels);  // resolution selector
  void setListener(Listener onChange) { _onChange = std::move(onChange); }

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override;
  void mouseExit(const juce::MouseEvent&) override;

private:
  juce::Rectangle<int> cellBounds(int index) const;
  int indexAt(juce::Point<int> pos) const;

  juce::StringArray _labels;
  int _columns;
  int _selected = 0;
  Listener _onChange;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RadioButtonMatrix)
};

}  // namespace neurythmic
