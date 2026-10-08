#pragma once

#include <functional>
#include <memory>

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "NetworkController.h"

namespace neurythmic {

// Serialises the network ValueTree to/from a single .nprs XML file. The tree
// IS XML internally, so save/load are cheap; the heavy lifting (engine rebuild)
// lives in NetworkController::rebuild().
class PresetManager {
public:
  explicit PresetManager(NetworkController& controller);

  bool savePreset(const juce::File& file);
  bool loadPreset(const juce::File& file);

  // Native async dialogs. The chooser is a top-level window (not parented to
  // the editor), so it is not clipped to the plugin's bounds.
  void browseForSave(std::function<void(const juce::File&)> onComplete = {});
  void browseForLoad(std::function<void(const juce::File&)> onComplete = {});

private:
  NetworkController& _controller;

  // Kept alive for the duration of the async dialog — JUCE's FileChooser must
  // outlive launchAsync() or the dialog is destroyed before it appears.
  std::shared_ptr<juce::FileChooser> _activeChooser;
};

}  // namespace neurythmic
