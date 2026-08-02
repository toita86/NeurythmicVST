#include "../include/Neurythmic/PluginEditor.h"

namespace neurythmic {
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p), _processor(p) {
  addAndMakeVisible(_startStopButton);
  setSize(600, 400);

  addAndMakeVisible(_node0);
  addAndMakeVisible(_node1);
  addAndMakeVisible(_node2);
  _node0.setLabel("Node 0 (root)");
  _node1.setLabel("Node 1");
  _node2.setLabel("Node 2");

  _startStopButton.setButtonText("Stop");
  _startStopButton.addListener(this);
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

void PluginEditor::resized() {
  auto area = getLocalBounds();

  area.removeFromTop(40);     // title space
  area.removeFromBottom(50);  // button space

  int eachWidth = area.getWidth() / 3;
  _node0.setBounds(area.removeFromLeft(eachWidth));
  _node1.setBounds(area.removeFromLeft(eachWidth));
  _node2.setBounds(area);

  _startStopButton.setBounds(getWidth() / 2 - 50, getHeight() - 40, 100, 30);
}

void PluginEditor::buttonClicked(juce::Button* b) {
  if (b == &_startStopButton) {
    if (_processor.isEngineRunning()) {
      _processor.stopEngine();
      _startStopButton.setButtonText("Start");
    } else {
      _processor.startEngine();
      _startStopButton.setButtonText("Stop");
    }
  }
}
}  // namespace neurythmic
