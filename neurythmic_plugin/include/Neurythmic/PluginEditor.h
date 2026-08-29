#pragma once

#include <array>
#include <memory>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "NodeComponent.h"
#include "PluginProcessor.h"

namespace neurythmic {
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::Button::Listener,
                     public juce::Timer {
public:
  explicit PluginEditor(
      PluginProcessor& p);  // explicit prevents accidental implicit conversion
                            // from a PluginProcessor& to a PluginEditor
  ~PluginEditor() override =
      default;  // tells the compiler to generate the default destructor.

  // Every Component (and AudioProcessorEditor is one) can draw itself. Override
  // paint() and JUCE calls it whenever the window needs to be redrawn:
  // Windowshows up → paint()
  // Window resized → paint()
  // You call repaint() → paint()
  // The juce::Graphics& g object is your drawing canvas.
  // It has its origin at (0, 0) = top-left corner of the component.
  void paint(juce::Graphics& g) override;

  void resized() override;

  void buttonClicked(juce::Button* b) override;

  void timerCallback() override;

private:
  PluginProcessor& _processor;  // read-only access to engine state
  juce::TextButton _startStopButton;
  juce::TextButton _createChildButton;
  std::vector<std::unique_ptr<NodeComponent>> _nodes;

  bool _rebuildNodes();
};
}  // namespace neurythmic
