#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "NetworkViewComponent.h"
#include "PluginProcessor.h"

namespace neurythmic {
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::Button::Listener,
                     public juce::Timer {
public:
  explicit PluginEditor(PluginProcessor& p);
  ~PluginEditor() override = default;

  void paint(juce::Graphics& g) override;
  void resized() override;

  void buttonClicked(juce::Button* b) override;
  void timerCallback() override;

private:
  PluginProcessor& _processor;
  NetworkViewComponent _networkView;
  juce::TextButton _createChildButton;
};
}  // namespace neurythmic
