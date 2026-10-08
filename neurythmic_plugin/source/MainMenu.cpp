#include "../include/Neurythmic/MainMenu.h"

#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

MainMenu::MainMenu(NetworkController& controller, PresetManager& presets)
    : _controller(controller), _presets(presets) {
  addAndMakeVisible(_tabCpg);
  addAndMakeVisible(_tabClose);
  addAndMakeVisible(_newPreset);
  addAndMakeVisible(_savePreset);
  addAndMakeVisible(_loadPreset);

  _tabCpg.setColour(juce::TextButton::buttonColourId,
                    MenuTheme::downBackground);
  _tabCpg.setColour(juce::TextButton::textColourOffId, MenuTheme::labelColour);

  _newPreset.onClick = [this] { _controller.clear(); };
  _savePreset.onClick = [this] { _presets.browseForSave(); };
  _loadPreset.onClick = [this] { _presets.browseForLoad(); };
  _tabClose.onClick = [this] {
    if (onClose)
      onClose();
  };
}

void MainMenu::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::guiBackground);
}

void MainMenu::resized() {
  auto area = getLocalBounds().reduced(6);

  auto tabRow = area.removeFromTop(26);
  auto cpgTab = tabRow.removeFromLeft(tabRow.getWidth() - 30);
  _tabCpg.setBounds(cpgTab.reduced(2));
  _tabClose.setBounds(tabRow.reduced(2));

  area.removeFromTop(8);

  _newPreset.setBounds(area.removeFromTop(28));
  area.removeFromTop(6);
  _savePreset.setBounds(area.removeFromTop(28));
  area.removeFromTop(6);
  _loadPreset.setBounds(area.removeFromTop(28));
}

}  // namespace neurythmic
