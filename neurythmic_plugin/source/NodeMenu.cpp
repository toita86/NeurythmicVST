#include "../include/Neurythmic/NodeMenu.h"

#include "../include/Neurythmic/MenuTheme.h"
#include "../include/Neurythmic/MenuValues.h"
#include "../include/Neurythmic/NetworkState.h"

namespace neurythmic {

namespace {

juce::StringArray buildFrequencyLabels() {
  juce::StringArray labels;
  for (int i = 0; i < MenuValues::frequencyCount(); ++i)
    labels.add(MenuValues::frequencyLabel(i));
  return labels;
}

juce::StringArray buildOctaveLabels() {
  juce::StringArray labels;
  for (int octave = -1; octave <= 5; ++octave)
    labels.add(juce::String(octave));
  return labels;
}

}  // namespace

NodeMenu::NodeMenu(NetworkController& controller, int nodeId)
    : _controller(controller),
      _nodeId(nodeId),
      _freqMatrix(buildFrequencyLabels(), 5),
      _syncMatrix({"NONE", "1X", "LOCK"}, 3),
      _gridMatrix({"OFF", "24th", "32nd"}, 3),
      _resolutionMatrix(juce::StringArray(), 4),
      _keyboard(_keyboardState,
                juce::MidiKeyboardComponent::horizontalKeyboard),
      _octaveMatrix(buildOctaveLabels(), 7) {
  setSize(kWidth, 400);

  addAndMakeVisible(_tabNode);
  addAndMakeVisible(_tabConstraint);
  addAndMakeVisible(_tabMidi);
  addAndMakeVisible(_freqMatrix);
  addAndMakeVisible(_fineTuneLabel);
  addAndMakeVisible(_fineTune);
  addAndMakeVisible(_selfNoiseLabel);
  addAndMakeVisible(_selfNoise);
  addAndMakeVisible(_phaseLabel);
  addAndMakeVisible(_phase);
  addAndMakeVisible(_syncLabel);
  addAndMakeVisible(_syncMatrix);
  addAndMakeVisible(_addChild);
  addAndMakeVisible(_deleteNode);
  addAndMakeVisible(_freedomLabel);
  addAndMakeVisible(_freedom);
  addAndMakeVisible(_gridLabel);
  addAndMakeVisible(_gridMatrix);
  addAndMakeVisible(_resolutionLabel);
  addAndMakeVisible(_resolutionMatrix);
  addAndMakeVisible(_constraintView);
  addAndMakeVisible(_keyboard);
  addAndMakeVisible(_octaveLabel);
  addAndMakeVisible(_octaveMatrix);
  addAndMakeVisible(_velocityLabel);
  addAndMakeVisible(_velocity);

  for (auto* label : {&_fineTuneLabel, &_selfNoiseLabel, &_phaseLabel,
                      &_syncLabel, &_freedomLabel, &_gridLabel,
                      &_resolutionLabel, &_octaveLabel, &_velocityLabel}) {
    label->setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    label->setColour(juce::Label::textColourId, MenuTheme::labelColour);
  }

  auto styleSlider = [](juce::Slider& s, double lo, double hi) {
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setRange(lo, hi);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  };
  styleSlider(_fineTune, 0.5, 2.0);
  styleSlider(_selfNoise, 0.0, 1.0);
  styleSlider(_phase, 0.0, 1.0);
  styleSlider(_freedom, 0.0, 1.0);
  styleSlider(_velocity, 0.0, 1.5);

  _tabNode.onClick = [this] { setTab(0); };
  _tabConstraint.onClick = [this] { setTab(1); };
  _tabMidi.onClick = [this] { setTab(2); };

  _freqMatrix.setListener([this](int idx) {
    const double rootFreq =
        _controller.getNodeFrequency(NetworkState::kRootNodeId);
    _controller.setNodeFrequency(
        _nodeId, rootFreq * MenuValues::frequencyMultiple(idx), false);
    refreshFrequency();
  });

  _fineTune.onValueChange = [this] {
    _controller.setNodeFrequency(_nodeId, _fineTune.getValue(), false);
  };

  _syncMatrix.setListener([this](int idx) {
    _controller.setNodeSynchMode(_nodeId, MenuValues::synchModeFromIndex(idx));
  });

  _selfNoise.onValueChange = [this] {
    _controller.setNodeSelfNoise(_nodeId, _selfNoise.getValue());
  };

  _phase.onValueChange = [this] {
    _controller.setNodePhaseOffset(_nodeId, _phase.getValue());
  };

  _addChild.onClick = [this] {
    _controller.createChild(_nodeId);
    dismiss();
  };

  _deleteNode.onClick = [this] {
    _controller.deleteNode(_nodeId);
    dismiss();
  };

  _freedom.onValueChange = [this] {
    _controller.setNodeQuantiseAmount(_nodeId, 1.0 - _freedom.getValue());
  };

  _gridMatrix.setListener([this](int idx) {
    _controller.setNodeQuantiseGrid(_nodeId, MenuValues::gridFromIndex(idx));
    refreshConstraint();
  });

  _resolutionMatrix.setListener([this](int idx) {
    const auto multiples = MenuValues::resolutionMultiples();
    _controller.setNodeQuantiseMultiple(_nodeId, multiples[idx]);
    _constraintView.setGridMultiplier(static_cast<int>(multiples[idx]));
  });

  // MIDI tab
  _keyboard.setAvailableRange(48, 71);  // C3..B4 (two octaves)
  _keyboard.setScrollButtonsVisible(false);
  _keyboard.setKeyWidth(14.0f);
  _keyboardState.addListener(this);

  _octaveMatrix.setListener([this](int idx) {
    const int octave = idx - 1;
    const int semitone =
        MenuValues::noteSemitone(_controller.getNodeMidiNote(_nodeId));
    _controller.setNodeMidiNote(
        _nodeId, MenuValues::midiNoteFromOctaveSemitone(octave, semitone));
    refreshMidi();
  });

  _velocity.onValueChange = [this] {
    _controller.setNodeVolume(_nodeId,
                              static_cast<float>(_velocity.getValue()));
  };

  // Delete is only valid for a non-root leaf node.
  bool deletable = _nodeId != NetworkState::kRootNodeId;
  if (deletable) {
    NetworkState::forEachNode(_controller.getTree(), [&](juce::ValueTree n) {
      if (static_cast<int>(n.getProperty(NetworkState::Props::parentId)) ==
          _nodeId)
        deletable = false;
    });
  }
  _deleteNode.setEnabled(deletable);

  refreshFrequency();
  refreshConstraint();
  refreshMidi();
  setTab(0);
}

void NodeMenu::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::guiBackground);
}

void NodeMenu::setTab(int tab) {
  _tab = tab;
  _tabNode.setColour(
      juce::TextButton::buttonColourId,
      tab == 0 ? MenuTheme::downBackground : MenuTheme::background);
  _tabConstraint.setColour(
      juce::TextButton::buttonColourId,
      tab == 1 ? MenuTheme::downBackground : MenuTheme::background);
  _tabMidi.setColour(
      juce::TextButton::buttonColourId,
      tab == 2 ? MenuTheme::downBackground : MenuTheme::background);

  const bool showNode = tab == 0;
  const bool showConstraint = tab == 1;
  const bool showMidi = tab == 2;

  _freqMatrix.setVisible(showNode);
  _fineTuneLabel.setVisible(showNode);
  _fineTune.setVisible(showNode);
  _selfNoiseLabel.setVisible(showNode);
  _selfNoise.setVisible(showNode);
  _phaseLabel.setVisible(showNode);
  _phase.setVisible(showNode);
  _syncLabel.setVisible(showNode);
  _syncMatrix.setVisible(showNode);
  _addChild.setVisible(showNode);
  _deleteNode.setVisible(showNode);

  _freedomLabel.setVisible(showConstraint);
  _freedom.setVisible(showConstraint);
  _gridLabel.setVisible(showConstraint);
  _gridMatrix.setVisible(showConstraint);
  _resolutionLabel.setVisible(showConstraint);
  _resolutionMatrix.setVisible(showConstraint);
  _constraintView.setVisible(showConstraint);

  _keyboard.setVisible(showMidi);
  _octaveLabel.setVisible(showMidi);
  _octaveMatrix.setVisible(showMidi);
  _velocityLabel.setVisible(showMidi);
  _velocity.setVisible(showMidi);

  resized();
}

void NodeMenu::refreshFrequency() {
  const double rootFreq =
      _controller.getNodeFrequency(NetworkState::kRootNodeId);
  const double freq = _controller.getNodeFrequency(_nodeId);
  const double mult = rootFreq > 0.0 ? freq / rootFreq : 1.0;
  _freqMatrix.setSelected(MenuValues::nearestFrequencyIndex(mult));
  _fineTune.setRange(freq / 2.0, freq * 2.0);
  _fineTune.setValue(freq, juce::dontSendNotification);
  _selfNoise.setValue(_controller.getNodeSelfNoise(_nodeId),
                      juce::dontSendNotification);
  _phase.setValue(_controller.getNodePhaseOffset(_nodeId),
                  juce::dontSendNotification);
  _syncMatrix.setSelected(
      MenuValues::synchModeToIndex(_controller.getNodeSynchMode(_nodeId)));
}

void NodeMenu::refreshConstraint() {
  const auto grid = _controller.getNodeQuantiseGrid(_nodeId);
  _gridMatrix.setSelected(MenuValues::gridToIndex(grid));

  auto labels = MenuValues::resolutionLabels(grid);
  if (labels.empty())
    labels = {"8", "4", "2", "1"};
  juce::StringArray labelArray;
  for (const auto& l : labels)
    labelArray.add(l);
  _resolutionMatrix.setLabels(labelArray);

  const float mult = _controller.getNodeQuantiseMultiple(_nodeId);
  const auto multiples = MenuValues::resolutionMultiples();
  int idx = 0;
  for (int i = 0; i < static_cast<int>(multiples.size()); ++i)
    if (static_cast<int>(multiples[i]) == static_cast<int>(mult))
      idx = i;
  _resolutionMatrix.setSelected(idx);

  _constraintView.setGrid(grid);
  _constraintView.setGridMultiplier(static_cast<int>(mult));
  _constraintView.setGridOffset(
      static_cast<int>(_controller.getNodeQuantiseOffset(_nodeId)));

  _freedom.setValue(1.0 - _controller.getNodeQuantiseAmount(_nodeId),
                    juce::dontSendNotification);
}

void NodeMenu::refreshMidi() {
  const int note = _controller.getNodeMidiNote(_nodeId);
  if (note < 0)
    return;

  const int octave = MenuValues::noteOctave(note);
  _octaveMatrix.setSelected(juce::jlimit(0, 6, octave + 1));
  _velocity.setValue(_controller.getNodeVolume(_nodeId),
                     juce::dontSendNotification);

  _updatingKeyboard = true;
  _keyboardState.allNotesOff(1);
  if (note >= 48 && note <= 71)
    _keyboardState.noteOn(1, note, 1.0f);
  _updatingKeyboard = false;
}

void NodeMenu::handleNoteOn(juce::MidiKeyboardState*,
                            int,
                            int midiNoteNumber,
                            float) {
  if (_updatingKeyboard)
    return;
  const int octave =
      MenuValues::noteOctave(_controller.getNodeMidiNote(_nodeId));
  const int semitone = MenuValues::noteSemitone(midiNoteNumber);
  _controller.setNodeMidiNote(
      _nodeId, MenuValues::midiNoteFromOctaveSemitone(octave, semitone));
  refreshMidi();
}

void NodeMenu::handleNoteOff(juce::MidiKeyboardState*, int, int, float) {}

void NodeMenu::dismiss() {
  if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
    box->dismiss();
}

void NodeMenu::resized() {
  auto area = getLocalBounds().reduced(6);

  auto tabRow = area.removeFromTop(26);
  _tabNode.setBounds(tabRow.removeFromLeft(tabRow.getWidth() / 3).reduced(2));
  _tabConstraint.setBounds(
      tabRow.removeFromLeft(tabRow.getWidth() / 2).reduced(2));
  _tabMidi.setBounds(tabRow.reduced(2));

  if (_tab == 0)
    layoutNodeTab();
  else if (_tab == 1)
    layoutConstraintTab();
  else
    layoutMidiTab();
}

void NodeMenu::layoutNodeTab() {
  auto area = getLocalBounds().reduced(6);
  area.removeFromTop(26 + 6);

  _freqMatrix.setBounds(area.removeFromTop(52));
  area.removeFromTop(6);

  auto row = area.removeFromTop(24);
  _fineTuneLabel.setBounds(row.removeFromLeft(60));
  _fineTune.setBounds(row);
  area.removeFromTop(4);

  row = area.removeFromTop(24);
  _selfNoiseLabel.setBounds(row.removeFromLeft(60));
  _selfNoise.setBounds(row);
  area.removeFromTop(4);

  row = area.removeFromTop(24);
  _phaseLabel.setBounds(row.removeFromLeft(60));
  _phase.setBounds(row);
  area.removeFromTop(4);

  row = area.removeFromTop(24);
  _syncLabel.setBounds(row.removeFromLeft(60));
  area.removeFromTop(4);

  _syncMatrix.setBounds(area.removeFromTop(26));
  area.removeFromTop(8);

  auto btnRow = area.removeFromTop(28);
  _addChild.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(2));
  _deleteNode.setBounds(btnRow.reduced(2));
}

void NodeMenu::layoutConstraintTab() {
  auto area = getLocalBounds().reduced(6);
  area.removeFromTop(26 + 6);

  auto row = area.removeFromTop(24);
  _freedomLabel.setBounds(row.removeFromLeft(60));
  _freedom.setBounds(row);
  area.removeFromTop(4);

  row = area.removeFromTop(24);
  _gridLabel.setBounds(row.removeFromLeft(60));
  area.removeFromTop(4);

  _gridMatrix.setBounds(area.removeFromTop(26));
  area.removeFromTop(6);

  row = area.removeFromTop(24);
  _resolutionLabel.setBounds(row.removeFromLeft(60));
  area.removeFromTop(4);

  _resolutionMatrix.setBounds(area.removeFromTop(26));
  area.removeFromTop(6);

  _constraintView.setBounds(area.removeFromTop(80));
}

void NodeMenu::layoutMidiTab() {
  auto area = getLocalBounds().reduced(6);
  area.removeFromTop(26 + 6);

  auto row = area.removeFromTop(24);
  _octaveLabel.setBounds(row.removeFromLeft(60));
  area.removeFromTop(4);

  _octaveMatrix.setBounds(area.removeFromTop(26));
  area.removeFromTop(6);

  row = area.removeFromTop(24);
  _velocityLabel.setBounds(row.removeFromLeft(60));
  _velocity.setBounds(row);
  area.removeFromTop(8);

  _keyboard.setBounds(area.removeFromTop(90));
}

}  // namespace neurythmic
