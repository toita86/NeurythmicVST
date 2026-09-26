#include "../include/Neurythmic/PluginProcessor.h"
#include "../include/Neurythmic/PluginEditor.h"
#include "../include/Neurythmic/ConfigManager.h"

namespace neurythmic {

// Constructor
PluginProcessor::PluginProcessor()
    : AudioProcessor(
          BusesProperties().withOutput("Output",
                                       juce::AudioChannelSet::stereo(),
                                       true)),
      _engine(44100,
              true,
              false,
              true),  // eventOnRise, fireOnPeak -> amplitude
      _controller(_engine, ConfigManager::get()),
      _midiOutput(ConfigManager::get().midiTriggerNote,
                  ConfigManager::get().midiDrumChannel),
      _apvts(*this, nullptr, "Parameters", createParameterLayout()),
      _defaultTempoSource(),
      _tempoSource(&_defaultTempoSource) {
  auto& cfg = ConfigManager::get();
  _engine.setParam_t2Overt1(cfg.t1Overt2);
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

  _engine.setEventCallback([this](int nodeId, float amplitude) {
    _midiOutput.fire(nodeId, amplitude, _currentSample);
  });
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

juce::AudioProcessorValueTreeState::ParameterLayout
PluginProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "rootFreq", "Root Frequency", juce::NormalisableRange<float>(0.1f, 20.0f),
      2.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "masterVolume", "Master Volume", 0.0f, 1.0f, 0.8f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "masterQuantAmount", "Quantise Amount", 0.0f, 1.0f, 0.7f));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      "velocityMode", "Velocity Mode (Amplitude)", true));
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "routingMode", "Routing Mode",
      juce::StringArray{"Drum Machine", "Per-channel"}, 0));
  return layout;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  process(midiMessages, buffer.getNumSamples());

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());
}

void PluginProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  process(midiMessages, buffer.getNumSamples());

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());
}

void PluginProcessor::process(juce::MidiBuffer& midiMessages, int numSamples) {
  _defaultTempoSource.setPlayhead(getPlayHead());
  _lastTempo = _tempoSource->read();

  if (_midiResetRequested.exchange(false))
    _midiOutput.allNotesOff(0);

  const bool run = !_lastTempo.hasHost || _lastTempo.isPlaying;

  _midiOutput.setMasterVolume(
      _apvts.getRawParameterValue("masterVolume")->load());
  _midiOutput.setVelocityMode(
      _apvts.getRawParameterValue("velocityMode")->load() > 0.5f
          ? VelocityMode::Amplitude
          : VelocityMode::Constant);
  _midiOutput.setRoutingMode(
      _apvts.getRawParameterValue("routingMode")->load() > 0.5f
          ? RoutingMode::PerChannel
          : RoutingMode::DrumMachine);

  if (run) {
    const float rootFreq = _apvts.getRawParameterValue("rootFreq")->load();
    const float quantAmount =
        _apvts.getRawParameterValue("masterQuantAmount")->load();
    const double freq = _lastTempo.isPlaying ? _lastTempo.bpm / 60.0 : rootFreq;

    _engine.setNodeFrequency(0, freq, false);
    _engine.setQuantiseAmount(quantAmount);
    _engine.doQueuedActions();
    for (int sample = 0; sample < numSamples; ++sample) {
      _currentSample = sample;
      _engine.step();
    }
    _midiOutput.flush(midiMessages);
  } else {
    _midiOutput.allNotesOff(0);
    _midiOutput.flush(midiMessages);
  }
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto xml = _controller.getTree().createXml();
  if (xml == nullptr)
    return;

  juce::MemoryOutputStream stream(destData, false);
  {
    juce::GZIPCompressorOutputStream gzip(stream, 9);
    xml->writeTo(gzip);
  }
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
  juce::GZIPDecompressorInputStream gzip(stream);

  const juce::String text = gzip.readEntireStreamAsString();
  auto xml = juce::parseXML(text);
  if (xml == nullptr)
    return;

  juce::ValueTree tree = juce::ValueTree::fromXml(*xml);
  if (!tree.isValid())
    return;

  _controller.rebuild(tree);
  _midiResetRequested.store(true);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
  return new PluginEditor(*this);  // uses the defined PluginEditor
}

bool PluginProcessor::hasEditor() const {
  return true;
}

// Phase 2 API
void PluginProcessor::setTempoSource(TempoSource* source) {
  _tempoSource = source;
}

bool PluginProcessor::isPlaying() const {
  return _lastTempo.isPlaying;
}

// Getters exposed for the editor
double PluginProcessor::getNodeFrequency(unsigned id) const {
  return _engine.getNodeFrequency(id);
}

}  // namespace neurythmic

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new neurythmic::PluginProcessor();
}
