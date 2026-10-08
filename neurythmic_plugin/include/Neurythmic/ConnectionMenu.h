#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "NetworkController.h"

namespace neurythmic {

// Right-click context menu for a connection (CallOutBox content): weight,
// phase and remove.
class ConnectionMenu : public juce::Component {
public:
  ConnectionMenu(NetworkController& controller, int from, int to);
  ~ConnectionMenu() override = default;

  void resized() override;
  void paint(juce::Graphics&) override;

  static constexpr int kWidth = 250;

private:
  void dismiss();

  NetworkController& _controller;
  int _from;
  int _to;

  juce::Label _weightLabel{"", "Weight"};
  juce::Slider _weight;
  juce::Label _phaseLabel{"", "Phase"};
  juce::Slider _phase;
  juce::TextButton _remove{"Remove Connection"};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionMenu)
};

}  // namespace neurythmic
