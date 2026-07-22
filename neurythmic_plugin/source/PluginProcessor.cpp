#include "../include/Neurythmic/PluginProcessor.h"
#include "../../CPGLib/MatsuokaEngine.h"

namespace neurythmic {

PluginProcessor::PluginProcessor()
    : AudioProcessor(
          BusesProperties().withOutput("Output",
                                       juce::AudioChannelSet::stereo(),
                                       true)) {}

PluginProcessor::~PluginProcessor() = default;

const juce::String PluginProcessor::getName() const {
  return NEURYTHMIC_PLUGIN_NAME;
}
bool PluginProcessor::acceptsMidi() const {
  return true;
}
bool PluginProcessor::producesMidi() const {
  return true;
}
bool PluginProcessor::isMidiEffect() const {
  return false;
}
double PluginProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int PluginProcessor::getNumPrograms() {
  return 1;
}
int PluginProcessor::getCurrentProgram() {
  return 0;
}
void PluginProcessor::setCurrentProgram(int) {}
const juce::String PluginProcessor::getProgramName(int) {
  return {};
}
void PluginProcessor::changeProgramName(int, const juce::String&) {}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  if (layouts.getNumChannels(true, 0) > 0)  // reject input buses
    return false;

  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());
}

void PluginProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::ignoreUnused(destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
  return new juce::GenericAudioProcessorEditor(*this);
}

bool PluginProcessor::hasEditor() const {
  return true;
}

}  // namespace neurythmic

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new neurythmic::PluginProcessor();
}
