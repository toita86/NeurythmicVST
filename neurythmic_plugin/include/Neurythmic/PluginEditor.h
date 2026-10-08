#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "IconButton.h"
#include "MainMenu.h"
#include "NetworkViewComponent.h"
#include "PluginProcessor.h"
#include "PresetManager.h"

namespace neurythmic {
class PluginEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
  explicit PluginEditor(PluginProcessor& p);
  ~PluginEditor() override = default;

  void paint(juce::Graphics& g) override;
  void resized() override;

  void timerCallback() override;

private:
  PluginProcessor& _processor;
  NetworkViewComponent _networkView;
  PresetManager _presets;
  MainMenu _mainMenu;
  IconButton _showMenuButton{IconButton::Icon::Hamburger};
};
}  // namespace neurythmic
