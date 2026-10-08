#include "../include/Neurythmic/PresetManager.h"

#include <functional>
#include <memory>

namespace neurythmic {
PresetManager::PresetManager(NetworkController& controller)
    : _controller(controller) {}

bool PresetManager::savePreset(const juce::File& file) {
  // juce::ValueTree is basically a xml
  auto xml = _controller.getTree().createXml();
  if (xml == nullptr)
    return false;
  return xml->writeTo(file);
}

bool PresetManager::loadPreset(const juce::File& file) {
  juce::XmlDocument doc(file);
  auto xml = doc.getDocumentElement();
  if (xml == nullptr)
    return false;
  juce::ValueTree tree = juce::ValueTree::fromXml(*xml);
  if (!tree.isValid())
    return false;
  _controller.rebuild(tree);
  return true;
}

void PresetManager::browseForSave(
    std::function<void(const juce::File&)> onComplete) {
  _activeChooser = std::make_shared<juce::FileChooser>(
      "Save preset...",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.nprs",
      false);
  _activeChooser->launchAsync(juce::FileBrowserComponent::saveMode |
                                  juce::FileBrowserComponent::canSelectFiles,
                              [this, onComplete](const juce::FileChooser& c) {
                                juce::File result = c.getResult();
                                if (result != juce::File()) {
                                  if (result.getFileExtension() != ".nprs")
                                    result = result.withFileExtension(".nprs");
                                  savePreset(result);
                                }
                                if (onComplete)
                                  onComplete(result);
                              });
}

void PresetManager::browseForLoad(
    std::function<void(const juce::File&)> onComplete) {
  _activeChooser = std::make_shared<juce::FileChooser>(
      "Load preset...",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.nprs",
      false);
  _activeChooser->launchAsync(juce::FileBrowserComponent::openMode |
                                  juce::FileBrowserComponent::canSelectFiles,
                              [this, onComplete](const juce::FileChooser& c) {
                                juce::File result = c.getResult();
                                if (result != juce::File())
                                  loadPreset(result);
                                if (onComplete)
                                  onComplete(result);
                              });
}

}  // namespace neurythmic
