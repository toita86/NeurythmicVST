#include "../include/Neurythmic/PluginEditor.h"

namespace neurythmic {
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p), _processor(p) {
  addAndMakeVisible(_createChildButton);
  _createChildButton.setButtonText("add child");
  _createChildButton.addListener(this);
  setSize(600, 400);
  _rebuildNodes();
  startTimerHz(30);
}

void PluginEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::darkgrey);

  g.setColour(juce::Colours::white);
  g.setFont(24.0f);
  g.drawText("Neurythmic CPG", getLocalBounds(),
             juce::Justification::centredTop);
}

// Returns true when the number of node components changed (so the caller knows
// to re-run resized() and give the new components real bounds).
bool PluginEditor::_rebuildNodes() {
  auto ids = _processor.getController().getNodeIds();
  const bool changed = _nodes.size() != ids.size();

  while (_nodes.size() < ids.size()) {
    auto node = std::make_unique<NodeComponent>();
    addAndMakeVisible(node.get());
    _nodes.push_back(std::move(node));
  }
  while (_nodes.size() > ids.size())
    _nodes.pop_back();  // unique_ptr dtor removes the Component from the parent

  for (size_t i = 0; i < _nodes.size(); ++i)
    _nodes[i]->setLabel("Node " + juce::String(ids[i]));

  return changed;
}

void PluginEditor::resized() {
  auto area = getLocalBounds();
  area.removeFromTop(40);
  area.removeFromBottom(50);

  auto& controller = _processor.getController();
  auto ids = controller.getNodeIds();
  const float halfW = 60.0f, halfH = 60.0f;
  for (size_t i = 0; i < _nodes.size(); ++i) {
    auto p = controller.getNodePosition(ids[i]);  // normalised 0..1
    int cx = area.getX() + static_cast<int>(p.getX() * area.getWidth());
    int cy = area.getY() + static_cast<int>(p.getY() * area.getHeight());
    _nodes[i]->setBounds(
        cx - static_cast<int>(halfW), cy - static_cast<int>(halfH),
        static_cast<int>(halfW * 2), static_cast<int>(halfH * 2));
  }

  // Non-overlapping buttons: the single create-child button near centre-bottom.
  _createChildButton.setBounds(getWidth() / 2 - 50, getHeight() - 40, 100, 30);
}

void PluginEditor::buttonClicked(juce::Button* b) {
  if (b == &_createChildButton) {
    _processor.getController().createChild(0);  // guard removed
  }
}

void PluginEditor::timerCallback() {
  if (!_processor.isPlaying())
    return;

  auto& controller = _processor.getController();
  controller.updateFromEngine();  // polls events, steps envelopes, syncs freq

  if (_rebuildNodes())
    resized();  // a node was added/removed → assign its bounds

  auto ids = controller.getNodeIds();
  for (size_t i = 0; i < _nodes.size(); ++i) {
    int id = ids[i];
    _nodes[i]->setFrequency(_processor.getNodeFrequency(id));
    _nodes[i]->setIntensity(controller.getNodeIntensity(id));
    _nodes[i]->repaint();
  }
}

}  // namespace neurythmic
