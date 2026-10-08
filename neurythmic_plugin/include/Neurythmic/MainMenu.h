#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "IconButton.h"
#include "NetworkController.h"
#include "PresetManager.h"

namespace neurythmic {

// Docked right sidebar (Phase 5). CPG tab holds preset management; the Mixer
// tab arrives in Phase 6. The ✕ tab fires `onClose` so the editor can hide it.
class MainMenu : public juce::Component {
public:
  MainMenu(NetworkController& controller, PresetManager& presets);
  ~MainMenu() override = default;

  void resized() override;
  void paint(juce::Graphics&) override;

  std::function<void()> onClose;

  static constexpr int kWidth = 270;

private:
  NetworkController& _controller;
  PresetManager& _presets;

  juce::TextButton _tabCpg{"CPG"};
  IconButton _tabClose{IconButton::Icon::Close};

  juce::TextButton _newPreset{"NEW Preset"};
  juce::TextButton _savePreset{"Save Preset File"};
  juce::TextButton _loadPreset{"Load Preset File"};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainMenu)
};

}  // namespace neurythmic
