#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "IconButton.h"
#include "MixerPanel.h"
#include "NetworkController.h"
#include "PluginProcessor.h"
#include "PresetManager.h"

namespace neurythmic {

// Docked right sidebar (Phase 5/6). Tabs: [CPG] preset management and
// [Mixer] per-node strips + master controls. The ✕ tab fires `onClose`.
class MainMenu : public juce::Component {
public:
  MainMenu(NetworkController& controller,
           PresetManager& presets,
           PluginProcessor& processor);
  ~MainMenu() override = default;

  void resized() override;
  void paint(juce::Graphics&) override;

  // Forwards to the mixer so labels/channels stay current (editor timer).
  void refresh();

  std::function<void()> onClose;

  static constexpr int kWidth = 270;

private:
  void setTab(int tab);

  NetworkController& _controller;
  PresetManager& _presets;
  int _tab = 0;

  juce::TextButton _tabCpg{"CPG"};
  juce::TextButton _tabMixer{"Mixer"};
  IconButton _tabClose{IconButton::Icon::Close};

  // CPG tab
  juce::TextButton _newPreset{"NEW Preset"};
  juce::TextButton _savePreset{"Save Preset File"};
  juce::TextButton _loadPreset{"Load Preset File"};

  // Mixer tab
  MixerPanel _mixer;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainMenu)
};

}  // namespace neurythmic
