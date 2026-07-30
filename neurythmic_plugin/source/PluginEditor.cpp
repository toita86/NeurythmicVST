#include "../include/Neurythmic/PluginEditor.h"

namespace neurythmic {
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p), _processor(p) {
  setSize(600, 400);
}

void PluginEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::darkgrey);

  g.setColour(juce::Colours::white);
  g.setFont(24.0f);
  g.drawText(
      "Neurythmic CPG", getLocalBounds(),
      juce::Justification::centredTop);  // centredTop = horizontally centered,
                                         // vertically at the top edge.
}
}  // namespace neurythmic
