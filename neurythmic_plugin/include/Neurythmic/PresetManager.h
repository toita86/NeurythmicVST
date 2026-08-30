#pragma once

#include <functional>
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

  // Native async dialogs .
  void browseForSave(std::function<void(const juce::File&)> onComplete = {});
  void browseForLoad(std::function<void(const juce::File&)> onComplete = {});

private:
  NetworkController& _controller;
};

}  // namespace neurythmic
