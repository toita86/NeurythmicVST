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

}  // namespace

NodeMenu::NodeMenu(NetworkController& controller, int nodeId)
    : _controller(controller),
      _nodeId(nodeId),
      _freqMatrix(buildFrequencyLabels(), 5),
      _syncMatrix({"NONE", "1X", "LOCK"}, 3),
      _gridMatrix({"OFF", "24th", "32nd"}, 3),
      _resolutionMatrix(juce::StringArray(), 4) {
  setSize(kWidth, 380);

  addAndMakeVisible(_tabNode);
  addAndMakeVisible(_tabConstraint);
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

  for (auto* label :
       {&_fineTuneLabel, &_selfNoiseLabel, &_phaseLabel, &_syncLabel,
        &_freedomLabel, &_gridLabel, &_resolutionLabel}) {
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

  _tabNode.onClick = [this] { setTab(0); };
  _tabConstraint.onClick = [this] { setTab(1); };

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

  const bool showNode = tab == 0;
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

  _freedomLabel.setVisible(!showNode);
  _freedom.setVisible(!showNode);
  _gridLabel.setVisible(!showNode);
  _gridMatrix.setVisible(!showNode);
  _resolutionLabel.setVisible(!showNode);
  _resolutionMatrix.setVisible(!showNode);
  _constraintView.setVisible(!showNode);

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

void NodeMenu::dismiss() {
  if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
    box->dismiss();
}

void NodeMenu::resized() {
  auto area = getLocalBounds().reduced(6);

  auto tabRow = area.removeFromTop(26);
  _tabNode.setBounds(tabRow.removeFromLeft(tabRow.getWidth() / 2).reduced(2));
  _tabConstraint.setBounds(tabRow.reduced(2));

  if (_tab == 0)
    layoutNodeTab();
  else
    layoutConstraintTab();
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

}  // namespace neurythmic
