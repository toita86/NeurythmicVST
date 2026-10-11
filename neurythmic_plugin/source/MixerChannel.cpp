#include "../include/Neurythmic/MixerChannel.h"

#include "../include/Neurythmic/ConfigManager.h"
#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

MixerChannel::MixerChannel(NetworkController& controller, int nodeId)
    : _controller(controller), _nodeId(nodeId) {
  _name.setText("Node " + juce::String(nodeId), juce::dontSendNotification);
  _name.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
  _name.setColour(juce::Label::textColourId, MenuTheme::labelColour);

  _volume.setSliderStyle(juce::Slider::LinearHorizontal);
  _volume.setRange(0.0, 1.5, 0.001);
  _volume.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  _volume.onValueChange = [this] {
    _controller.setNodeVolume(_nodeId, static_cast<float>(_volume.getValue()));
    _value.setText(juce::String(_volume.getValue(), 2),
                   juce::dontSendNotification);
  };

  _mute.setClickingTogglesState(true);
  _mute.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff943137));
  _mute.onClick = [this] {
    _controller.setNodeMuted(_nodeId, _mute.getToggleState());
  };

  _solo.setClickingTogglesState(true);
  _solo.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd6c000));
  _solo.onClick = [this] {
    _controller.setNodeSoloed(_nodeId, _solo.getToggleState());
  };

  _value.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
  _value.setColour(juce::Label::textColourId, MenuTheme::labelColour);
  _value.setJustificationType(juce::Justification::centredRight);

  addAndMakeVisible(_name);
  addAndMakeVisible(_volume);
  addAndMakeVisible(_mute);
  addAndMakeVisible(_solo);
  addAndMakeVisible(_value);

  refresh();
}

void MixerChannel::refresh() {
  _volume.setValue(_controller.getNodeVolume(_nodeId),
                   juce::dontSendNotification);
  _mute.setToggleState(_controller.getNodeMuted(_nodeId),
                       juce::dontSendNotification);
  _solo.setToggleState(_controller.getNodeSoloed(_nodeId),
                       juce::dontSendNotification);
  _value.setText(juce::String(_volume.getValue(), 2),
                 juce::dontSendNotification);
  repaint();
}

void MixerChannel::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::background);
  g.setColour(ConfigManager::get().getNodeColour(_nodeId));
  g.fillRect(0, 0, 4, getHeight());
  g.setColour(juce::Colour(0xff1e2226));
  g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
}

void MixerChannel::resized() {
  auto area = getLocalBounds().reduced(4, 2);
  area.removeFromLeft(4);

  _name.setBounds(area.removeFromLeft(52));
  _value.setBounds(area.removeFromRight(34));

  auto buttons = area.removeFromRight(52);
  _mute.setBounds(buttons.removeFromLeft(24).reduced(1));
  _solo.setBounds(buttons.removeFromLeft(24).reduced(1));

  area.removeFromRight(4);
  _volume.setBounds(area);
}

}  // namespace neurythmic
