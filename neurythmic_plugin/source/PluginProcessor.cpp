#include "../include/Neurythmic/PluginProcessor.h"
#include "../include/Neurythmic/PluginEditor.h"
#include "../include/Neurythmic/ConfigManager.h"

namespace neurythmic {

// Contructor
PluginProcessor::PluginProcessor()
    : AudioProcessor(
          BusesProperties().withOutput("Output",
                                       juce::AudioChannelSet::stereo(),
                                       true)),
      _engine(44100) {
  // Using the ConfigManager to setup the Matshuoka Engines
  auto& cfg = ConfigManager::get();
  _engine.setParam_c(cfg.c);
  _engine.setParam_b(cfg.b);
  _engine.setParam_g(cfg.g);
  _engine.setFreqCompensation(cfg.freqCompensation);
  _engine.setConnectionWeightScaling(cfg.connectionWeightScalingOn);
  _engine.setUnityConnectionWeight(cfg.connectionWeightScalingUnity);
  _engine.loadConnectionWeightCurve(cfg.getWeightScalingCurveX(),
                                    cfg.getWeightScalingCurveY());
  _engine.doQueuedActions();
  _engine.calibrate();

  _setupNetwork();
}

PluginProcessor::~PluginProcessor() = default;

// PUBLIC
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
  _engine.setSampleRate(static_cast<unsigned>(sampleRate));
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

  if (_running) {
    _engine.doQueuedActions();

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      _engine.step();

    auto events = _engine.getEvents();
    for (auto& e : events) {
      if (e.nodeID < 16)
        _nodeFired[e.nodeID] = true;
    }
  }

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }
}

void PluginProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;

  if (_running) {
    _engine.doQueuedActions();

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      _engine.step();

    auto events = _engine.getEvents();
    for (auto& e : events) {
      if (e.nodeID < 16)
        _nodeFired[e.nodeID] = true;
    }
  }

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::ignoreUnused(destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
  return new PluginEditor(*this);  // uses the defined PluginEditor
}

bool PluginProcessor::hasEditor() const {
  return true;
}

std::array<bool, 16> PluginProcessor::popFiredNodes() {
  auto copy = _nodeFired;
  _nodeFired.fill(false);
  return copy;
}

// Getters exposed for the editor
int PluginProcessor::getNodeCount() const {
  return _engine.getNodeList().size();
}
int PluginProcessor::getNodeSignalState(u_int id) const {
  return static_cast<int>(_engine.getNode(id).getSignalState());
}
double PluginProcessor::getNodeOutput(u_int id) const {
  return _engine.getNode(id).getOutput();
}
double PluginProcessor::getNodeFrequency(u_int id) const {
  return _engine.getNodeFrequency(id);
}
bool PluginProcessor::isEngineRunning() const {
  return _running;
}
void PluginProcessor::startEngine() {
  _running = true;
}
void PluginProcessor::stopEngine() {
  _running = false;
}

// PRIVATE
void PluginProcessor::_setupNetwork() {
  _engine.addChild(0, 1);     // node 1 is child of root
  _engine.addChild(0, 2);     // node 2 is child of root
  _engine.doQueuedActions();  // <-- THIS applies the additions
  /*
  addChild is a QUEUED ACTION. Until doQueuedActions(), the network still has
  only node 0. After, it has 3 nodes. doQueuedActions() is the gate.
  */

  _engine.setNodeFrequency(0, 2.0, false);
  _engine.setNodeFrequency(1, 2.7, false);
  _engine.setNodeFrequency(2, 3.3, false);
  _engine.setConnection(0, 1, 0.5);
  _engine.setConnection(0, 2, 0.25);

  // _engine.setNodeQuantiser_Grid(1, MatsuokaEngine::gridType::_24th);
  // _engine.setNodeQuantiser_Grid(2, MatsuokaEngine::gridType::_24th);

  _engine.doQueuedActions();
}

}  // namespace neurythmic

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new neurythmic::PluginProcessor();
}
