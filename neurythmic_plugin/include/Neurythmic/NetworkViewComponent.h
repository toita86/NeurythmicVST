#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ConfigManager.h"
#include "NetworkController.h"

namespace neurythmic {

// Software renderer for the network graph. Draws nodes, parent-child straight
// edges, curved input edges, arrowheads and node labels using juce::Graphics,
// reusing the GraphGeometry helpers for the geometry and visual mapping.
class NetworkViewComponent : public juce::Component {
public:
  explicit NetworkViewComponent(NetworkController& controller);

  // Requests a repaint. Called from the editor's timer each frame.
  void update();

  void paint(juce::Graphics& g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;

private:
  void drawConnections(juce::Graphics& g,
                       const ConfigManager& cfg,
                       float scaling,
                       float minDim);
  void drawNodes(juce::Graphics& g,
                 const ConfigManager& cfg,
                 float scaling,
                 float minDim);
  void drawLabels(juce::Graphics& g,
                  const ConfigManager& cfg,
                  float scaling,
                  float minDim);

  juce::Point<float> toPixel(juce::Point<float> normalised, float minDim) const;
  juce::Point<float> toNormalised(juce::Point<float> pixel, float minDim) const;
  void focusNode(int nodeId, juce::Point<float> pixel);
  void focusConnection(int from, int to, juce::Point<float> pixel);

  NetworkController& _controller;
  juce::Font _font{juce::FontOptions{}};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkViewComponent)
};

}  // namespace neurythmic
