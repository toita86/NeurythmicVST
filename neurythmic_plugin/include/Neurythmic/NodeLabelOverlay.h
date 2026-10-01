#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ConfigManager.h"
#include "NetworkController.h"

namespace neurythmic {

// Software overlay drawn on top of the OpenGL network view. Paints the
// frequency-multiple label above each node and the quantise bar-division label
// below it, using plain juce::Graphics (kept out of the GL mesh layer).
class NodeLabelOverlay : public juce::Component {
public:
  explicit NodeLabelOverlay(NetworkController& controller);

  void paint(juce::Graphics& g) override;

private:
  NetworkController& _controller;
  juce::Font _font{juce::FontOptions{}};
};

}  // namespace neurythmic
