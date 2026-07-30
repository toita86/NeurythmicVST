#include "../include/Neurythmic/PluginEditor.h"

namespace neurythmic {
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p), _processor(p) {
  addAndMakeVisible(_startStopButton);
  setSize(600, 400);
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
  _startStopButton.setBounds(250, 350, 100, 30);
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
