#pragma once

#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "MixerChannel.h"
#include "NetworkController.h"
#include "PluginProcessor.h"

namespace neurythmic {

// Mixer tab content: a scrollable column of per-node MixerChannel strips plus
// master controls (master volume bound to the APVTS, read-only BPM/root-freq
// labels, and a solo-clear button).
class MixerPanel : public juce::Component {
public:
  MixerPanel(NetworkController& controller, PluginProcessor& processor);
  ~MixerPanel() override = default;

  void resized() override;
  void paint(juce::Graphics&) override;

  // Rebuilds the channel strips when the node list changes and refreshes the
  // read-only labels. Cheap when nothing changed (called from the editor
  // timer).
  void refresh();

private:
  void rebuildChannels();

  NetworkController& _controller;
  PluginProcessor& _processor;

  juce::Viewport _viewport;
  juce::Component _holder;
  std::vector<std::unique_ptr<MixerChannel>> _channels;
  std::vector<int> _nodeIds;

  juce::Label _masterLabel{"", "MASTER"};
  juce::Slider _masterVolume;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      _masterAttachment;

  juce::TextButton _clearSolo{"Clear Solo"};
  juce::Label _bpmLabel;
  juce::Label _rootLabel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};

}  // namespace neurythmic
