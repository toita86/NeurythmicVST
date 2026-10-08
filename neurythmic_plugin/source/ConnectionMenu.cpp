#include "../include/Neurythmic/ConnectionMenu.h"

#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

ConnectionMenu::ConnectionMenu(NetworkController& controller, int from, int to)
    : _controller(controller), _from(from), _to(to) {
  setSize(kWidth, 150);

  addAndMakeVisible(_weightLabel);
  addAndMakeVisible(_weight);
  addAndMakeVisible(_phaseLabel);
  addAndMakeVisible(_phase);
  addAndMakeVisible(_remove);

  for (auto* label : {&_weightLabel, &_phaseLabel}) {
    label->setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    label->setColour(juce::Label::textColourId, MenuTheme::labelColour);
  }

  auto& cfg = ConfigManager::get();
  _weight.setSliderStyle(juce::Slider::LinearHorizontal);
  _weight.setRange(0.0, static_cast<double>(cfg.connectionWeightMax));
  _weight.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  _weight.setValue(_controller.getConnectionScaleFactor(_from, _to),
                   juce::dontSendNotification);
  _weight.onValueChange = [this] {
    _controller.setConnectionScaleFactor(_from, _to, _weight.getValue());
  };

  _phase.setSliderStyle(juce::Slider::LinearHorizontal);
  _phase.setRange(0.0, 1.0);
  _phase.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  _phase.setValue(_controller.getConnectionPhase(_from, _to),
                  juce::dontSendNotification);
  _phase.onValueChange = [this] {
    _controller.updateConnectionPhase(_from, _to, _phase.getValue());
  };

  _remove.onClick = [this] {
    _controller.removeConnection(_from, _to);
    dismiss();
  };

  // Parent edges cannot be removed.
  if (_controller.getNodeParent(_to) == _from)
    _remove.setEnabled(false);
}

void ConnectionMenu::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::guiBackground);
}

void ConnectionMenu::dismiss() {
  if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
    box->dismiss();
}

void ConnectionMenu::resized() {
  auto area = getLocalBounds().reduced(8);

  auto row = area.removeFromTop(24);
  _weightLabel.setBounds(row.removeFromLeft(55));
  _weight.setBounds(row);
  area.removeFromTop(8);

  row = area.removeFromTop(24);
  _phaseLabel.setBounds(row.removeFromLeft(55));
  _phase.setBounds(row);
  area.removeFromTop(8);

  _remove.setBounds(area.removeFromTop(28));
}

}  // namespace neurythmic
