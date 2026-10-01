#include "../include/Neurythmic/PluginEditor.h"

namespace neurythmic {
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p),
      _processor(p),
      _networkView(_processor.getController()) {
  addAndMakeVisible(_networkView);
  addAndMakeVisible(_createChildButton);
  _createChildButton.setButtonText("add child");
  _createChildButton.addListener(this);
  setSize(500, 500);
  startTimerHz(30);
}

void PluginEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colour(25, 25, 30));
}

void PluginEditor::resized() {
  _networkView.setBounds(getLocalBounds());
  _createChildButton.setBounds(getWidth() / 2 - 50, getHeight() - 40, 100, 30);
  _networkView.update();
}

void PluginEditor::buttonClicked(juce::Button* b) {
  if (b == &_createChildButton) {
    _processor.getController().createChild(0);
    _networkView.update();
  }
}

void PluginEditor::timerCallback() {
  if (_processor.isPlaying())
    _processor.getController().updateFromEngine();

  // Always refresh the view so it renders (and re-renders) regardless of
  // transport state. The view rebuilds its scene and triggers a GL repaint.
  _networkView.update();
}

}  // namespace neurythmic
