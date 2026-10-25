#include "../include/Neurythmic/PresetManager.h"

#include <functional>
#include <memory>

#include "../include/Neurythmic/PluginProcessor.h"

namespace neurythmic {
PresetManager::PresetManager(NetworkController& controller,
                             PluginProcessor* processor)
    : _controller(controller), _processor(processor) {}

bool PresetManager::savePreset(const juce::File& file) {
  // Full plugin state when the processor is available (network + APVTS), so
  // root/master/trigger controls survive a .nprs round-trip. The
  // controller-only fallback keeps headless tests and embedders working.
  auto state = _processor ? _processor->createStateTree()
                          : _controller.getTree().createCopy();
  auto xml = state.createXml();
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
  if (_processor)
    _processor->restoreStateTree(tree);
  else
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
