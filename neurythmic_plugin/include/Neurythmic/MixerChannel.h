#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "NetworkController.h"

namespace neurythmic {

// One mixer strip: node name, horizontal volume fader (shared with the MIDI
// tab velocity), mute and solo toggles, and a volume value label.
class MixerChannel : public juce::Component {
public:
  MixerChannel(NetworkController& controller, int nodeId);
  ~MixerChannel() override = default;

  void resized() override;
  void paint(juce::Graphics&) override;

  // Pulls the current values from the controller (called when a refresh is
  // needed, e.g. after a preset load).
  void refresh();

  static constexpr int kHeight = 30;
  int getNodeId() const { return _nodeId; }

private:
  NetworkController& _controller;
  int _nodeId;

  juce::Label _name;
  juce::Slider _volume;
  juce::TextButton _mute{"M"};
  juce::TextButton _solo{"S"};
  juce::Label _value;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};

}  // namespace neurythmic
