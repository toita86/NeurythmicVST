#include "../include/Neurythmic/MixerPanel.h"

#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

MixerPanel::MixerPanel(NetworkController& controller,
                       PluginProcessor& processor)
    : _controller(controller), _processor(processor) {
  _viewport.setViewedComponent(&_holder, false);
  _viewport.setScrollBarsShown(true, false);
  addAndMakeVisible(_viewport);

  _masterLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
  _masterLabel.setColour(juce::Label::textColourId, MenuTheme::labelColour);

  _masterVolume.setSliderStyle(juce::Slider::LinearHorizontal);
  _masterVolume.setTextBoxStyle(juce::Slider::TextBoxRight, false, 46, 18);
  _masterAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          _processor.getAPVTS(), "masterVolume", _masterVolume);

  _clearSolo.onClick = [this] { _controller.clearSolo(); };

  _bpmLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
  _bpmLabel.setColour(juce::Label::textColourId, MenuTheme::labelColour);
  _rootLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
  _rootLabel.setColour(juce::Label::textColourId, MenuTheme::labelColour);

  addAndMakeVisible(_masterLabel);
  addAndMakeVisible(_masterVolume);
  addAndMakeVisible(_clearSolo);
  addAndMakeVisible(_bpmLabel);
  addAndMakeVisible(_rootLabel);

  refresh();
}

void MixerPanel::refresh() {
  const std::vector<int> ids = _controller.getNodeIds();
  if (ids != _nodeIds) {
    _nodeIds = ids;
    rebuildChannels();
  }
  for (auto& channel : _channels)
    channel->refresh();

  _bpmLabel.setText("BPM " + juce::String(_processor.getBpm(), 1),
                    juce::dontSendNotification);
  _rootLabel.setText(
      "Root " + juce::String(_processor.getRootFreq(), 2) + " Hz",
      juce::dontSendNotification);
}

void MixerPanel::rebuildChannels() {
  _channels.clear();
  for (int id : _nodeIds) {
    auto channel = std::make_unique<MixerChannel>(_controller, id);
    _holder.addAndMakeVisible(*channel);
    _channels.push_back(std::move(channel));
  }
  resized();
}

void MixerPanel::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::guiBackground);
}

void MixerPanel::resized() {
  auto area = getLocalBounds().reduced(4);

  auto bottom = area.removeFromBottom(88);
  _masterLabel.setBounds(bottom.removeFromTop(16));
  _masterVolume.setBounds(bottom.removeFromTop(22));
  bottom.removeFromTop(4);
  auto infoRow = bottom.removeFromTop(16);
  _bpmLabel.setBounds(infoRow.removeFromLeft(infoRow.getWidth() / 2));
  _rootLabel.setBounds(infoRow);
  bottom.removeFromTop(4);
  _clearSolo.setBounds(bottom.removeFromTop(22));

  _viewport.setBounds(area);
  _holder.setSize(area.getWidth() - 10,
                  static_cast<int>(_channels.size()) * MixerChannel::kHeight);
  int y = 0;
  for (auto& channel : _channels) {
    channel->setBounds(0, y, _holder.getWidth(), MixerChannel::kHeight);
    y += MixerChannel::kHeight;
  }
}

}  // namespace neurythmic
