#include "../include/Neurythmic/MainMenu.h"

#include "../include/Neurythmic/MenuTheme.h"

namespace neurythmic {

MainMenu::MainMenu(NetworkController& controller,
                   PresetManager& presets,
                   PluginProcessor& processor)
    : _controller(controller),
      _presets(presets),
      _mixer(controller, processor) {
  addAndMakeVisible(_tabCpg);
  addAndMakeVisible(_tabMixer);
  addAndMakeVisible(_tabClose);
  addAndMakeVisible(_newPreset);
  addAndMakeVisible(_savePreset);
  addAndMakeVisible(_loadPreset);
  addAndMakeVisible(_mixer);

  for (auto* tab : {&_tabCpg, &_tabMixer}) {
    tab->setColour(juce::TextButton::buttonColourId, MenuTheme::background);
    tab->setColour(juce::TextButton::textColourOffId, MenuTheme::labelColour);
  }

  _newPreset.onClick = [this] { _controller.clear(); };
  _savePreset.onClick = [this] { _presets.browseForSave(); };
  _loadPreset.onClick = [this] { _presets.browseForLoad(); };
  _tabClose.onClick = [this] {
    if (onClose)
      onClose();
  };

  _tabCpg.onClick = [this] { setTab(0); };
  _tabMixer.onClick = [this] { setTab(1); };

  setTab(0);
}

void MainMenu::paint(juce::Graphics& g) {
  g.fillAll(MenuTheme::guiBackground);
}

void MainMenu::setTab(int tab) {
  _tab = tab;
  _tabCpg.setColour(
      juce::TextButton::buttonColourId,
      tab == 0 ? MenuTheme::downBackground : MenuTheme::background);
  _tabMixer.setColour(
      juce::TextButton::buttonColourId,
      tab == 1 ? MenuTheme::downBackground : MenuTheme::background);

  const bool cpg = tab == 0;
  _newPreset.setVisible(cpg);
  _savePreset.setVisible(cpg);
  _loadPreset.setVisible(cpg);
  _mixer.setVisible(!cpg);
  resized();
}

void MainMenu::refresh() {
  _mixer.refresh();
}

void MainMenu::resized() {
  auto area = getLocalBounds().reduced(6);

  auto tabRow = area.removeFromTop(26);
  auto closeArea = tabRow.removeFromRight(30);
  _tabClose.setBounds(closeArea.reduced(2));
  _tabCpg.setBounds(tabRow.removeFromLeft(tabRow.getWidth() / 2).reduced(2));
  _tabMixer.setBounds(tabRow.reduced(2));

  area.removeFromTop(8);

  if (_tab == 0) {
    _newPreset.setBounds(area.removeFromTop(28));
    area.removeFromTop(6);
    _savePreset.setBounds(area.removeFromTop(28));
    area.removeFromTop(6);
    _loadPreset.setBounds(area.removeFromTop(28));
  } else {
    _mixer.setBounds(area);
  }
}

}  // namespace neurythmic
