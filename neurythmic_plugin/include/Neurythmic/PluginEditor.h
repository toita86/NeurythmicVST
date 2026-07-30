#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace neurythmic {
class PluginEditor : public juce::AudioProcessorEditor {
public:
  explicit PluginEditor(
      PluginProcessor& p);  // explicit prevents accidental implicit conversion
                            // from a PluginProcessor& to a PluginEditor
  ~PluginEditor() override =
      default;  // tells the compiler to generate the default destructor.

private:
  PluginProcessor& _processor;  // read-only access to engine state
};
}  // namespace neurythmic