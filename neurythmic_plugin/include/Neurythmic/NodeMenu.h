#pragma once

#include <functional>

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "ConstraintGUI.h"
#include "NetworkController.h"
#include "RadioButtonMatrix.h"

namespace neurythmic {

// Right-click context menu for a node (CallOutBox content). Tabbed:
//   [Node]        — frequency matrix, fine tune, sync mode, self-noise, phase,
//                   add/delete child
//   [Constraint]  — freedom, grid type, resolution, constraint grid view
//   [MIDI]        — per-node note (piano + octave) and shared velocity
class NodeMenu : public juce::Component,
                 private juce::MidiKeyboardState::Listener {
public:
  NodeMenu(NetworkController& controller, int nodeId);
  ~NodeMenu() override = default;

  void resized() override;
  void paint(juce::Graphics&) override;

  static constexpr int kWidth = 250;

private:
  void setTab(int tab);
  void refreshFrequency();
  void refreshConstraint();
  void refreshMidi();
  void layoutNodeTab();
  void layoutConstraintTab();
  void layoutMidiTab();
  void dismiss();

  void handleNoteOn(juce::MidiKeyboardState* source,
                    int midiChannel,
                    int midiNoteNumber,
                    float velocity) override;
  void handleNoteOff(juce::MidiKeyboardState* source,
                     int midiChannel,
                     int midiNoteNumber,
                     float velocity) override;

  NetworkController& _controller;
  int _nodeId;
  bool _isRoot = false;
  int _tab = 0;
  bool _updatingKeyboard = false;

  // tab bar
  juce::TextButton _tabNode{"Node"};
  juce::TextButton _tabConstraint{"Constraint"};
  juce::TextButton _tabMidi{"MIDI"};

  // Node tab
  juce::Label _rootHint;
  RadioButtonMatrix _freqMatrix;
  juce::Label _fineTuneLabel{"", "Fine Tune"};
  juce::Slider _fineTune;
  juce::Label _selfNoiseLabel{"", "Self-Noise"};
  juce::Slider _selfNoise;
  juce::Label _phaseLabel{"", "Phase"};
  juce::Slider _phase;
  juce::Label _syncLabel{"", "Sync Mode"};
  RadioButtonMatrix _syncMatrix;
  juce::TextButton _addChild{"Add Child to Node"};
  juce::TextButton _deleteNode{"Delete Node"};

  // Constraint tab
  juce::Label _freedomLabel{"", "Freedom"};
  juce::Slider _freedom;
  juce::Label _gridLabel{"", "Grid"};
  juce::TextButton _gridHost{"Follow Host"};
  RadioButtonMatrix _gridMatrix;
  juce::Label _resolutionLabel{"", "Resolution"};
  RadioButtonMatrix _resolutionMatrix;
  ConstraintGUI _constraintView;

  // MIDI tab
  juce::MidiKeyboardState _keyboardState;
  juce::MidiKeyboardComponent _keyboard;
  juce::Label _octaveLabel{"", "Octave"};
  RadioButtonMatrix _octaveMatrix;
  juce::Label _velocityLabel{"", "Velocity"};
  juce::Slider _velocity;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodeMenu)
};

}  // namespace neurythmic
