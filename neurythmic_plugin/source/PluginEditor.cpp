#include "../include/Neurythmic/PluginEditor.h"

namespace neurythmic {
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p),
      _processor(p),
      _networkView(_processor.getController()),
      _presets(_processor.getController(), &_processor),
      _mainMenu(_processor.getController(), _presets, _processor) {
  addAndMakeVisible(_networkView);
  addAndMakeVisible(_mainMenu);
  addAndMakeVisible(_showMenuButton);

  _showMenuButton.onClick = [this] {
    _mainMenu.setVisible(true);
    _showMenuButton.setVisible(false);
    resized();
  };
  _mainMenu.onClose = [this] {
    _mainMenu.setVisible(false);
    _showMenuButton.setVisible(true);
    resized();
  };

  setResizable(true, true);
  setResizeLimits(700, 500, 2560, 1600);
  setSize(1000, 700);
  startTimerHz(30);
}

void PluginEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colour(25, 25, 30));
}

void PluginEditor::resized() {
  if (_mainMenu.isVisible()) {
    _mainMenu.setBounds(getWidth() - MainMenu::kWidth, 0, MainMenu::kWidth,
                        getHeight());
    _networkView.setBounds(0, 0, getWidth() - MainMenu::kWidth, getHeight());
    _showMenuButton.setVisible(false);
  } else {
    _networkView.setBounds(getLocalBounds());
    _showMenuButton.setBounds(getWidth() - 30, 0, 30, 26);
  }
  _networkView.update();
}

void PluginEditor::timerCallback() {
  // Visual event draining and host-grid application live in the processor's
  // message-thread timer, so they keep working with the editor closed (and in
  // standalone, where isPlaying() is false). The editor only renders.
  _networkView.update();
  if (_mainMenu.isVisible())
    _mainMenu.refresh();
}

}  // namespace neurythmic
