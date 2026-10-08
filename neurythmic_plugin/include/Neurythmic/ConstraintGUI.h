#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MatsuokaEngine.h"

namespace neurythmic {

// Horizontal strip showing the quantise grid for the constraint tab. Draws a
// number of vertical grid lines (24 or 32), highlighting the current offset.
// Non-interactive for now (drag-to-set-offset is a future enhancement).
class ConstraintGUI : public juce::Component {
public:
  ConstraintGUI();

  void setGrid(MatsuokaEngine::gridType grid);
  void setGridMultiplier(int mult);
  void setGridOffset(int off);

  void paint(juce::Graphics&) override;

private:
  int _gridlineCount = 0;
  int _multiple = 8;
  int _offset = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstraintGUI)
};

}  // namespace neurythmic
