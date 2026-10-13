#include "../include/Neurythmic/PluginProcessor.h"
#include "../include/Neurythmic/PluginEditor.h"
#include "../include/Neurythmic/ConfigManager.h"
#include "../include/Neurythmic/NetworkState.h"

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

  // Phase 7: mirror per-node parameter changes into the APVTS automation
  // facade, and listen for host automation to push back into the controller.
  _controller.setOnNetworkChanged([this] { syncAllNodeParams(); });
  for (int id = 0; id < NetworkState::kMaxNodes; ++id) {
    if (id != NetworkState::kRootNodeId)
      _apvts.addParameterListener(nodeParamId(id, "freq"), this);
    _apvts.addParameterListener(nodeParamId(id, "volume"), this);
    _apvts.addParameterListener(nodeParamId(id, "pitch"), this);
    _apvts.addParameterListener(nodeParamId(id, "mute"), this);
    _apvts.addParameterListener(nodeParamId(id, "quantAmount"), this);
  }
  syncAllNodeParams();
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
  auto& cfg = ConfigManager::get();
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

  // Phase 7: fixed 16-slot pool of per-node automation parameters. JUCE cannot
  // remove parameters, so a stable pool is the only automation-safe option;
  // slots for non-existent nodes stay inert but present.
  for (int id = 0; id < NetworkState::kMaxNodes; ++id) {
    const juce::String label = "Node " + juce::String(id) + " ";
    if (id != NetworkState::kRootNodeId)  // root freq is the global rootFreq
      layout.add(std::make_unique<juce::AudioParameterFloat>(
          nodeParamId(id, "freq"), label + "Freq",
          juce::NormalisableRange<float>(0.25f, 16.0f),
          cfg.newNodeFreqMultiple));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        nodeParamId(id, "volume"), label + "Volume",
        juce::NormalisableRange<float>(0.0f, 1.5f), cfg.newNodeVolumeInit));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        nodeParamId(id, "pitch"), label + "Pitch", 0, 127,
        cfg.midiTriggerNote + id));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        nodeParamId(id, "mute"), label + "Mute", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        nodeParamId(id, "quantAmount"), label + "Quantise", 0.0f, 1.0f, 1.0f));
  }
  return layout;
}

juce::String PluginProcessor::nodeParamId(int nodeId, const char* field) {
  return "node_" + juce::String(nodeId).paddedLeft('0', 2) + "_" + field;
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

  _hostTimeSigNumerator.store(_lastTempo.timeSigNumerator);
  _hostTimeSigDenominator.store(_lastTempo.timeSigDenominator);

  if (_midiResetRequested.exchange(false))
    _midiOutput.allNotesOff(0);

  // Loop / seek detection: while the transport is already rolling, a backward
  // or oversized ppq jump means the host jumped position, so reset the engine
  // and release held notes. A stop->start transition is not a seek, so the
  // engine resumes without a reset (per the Phase 7 decision).
  if (_lastTempo.hasPpq) {
    if (_wasPlaying && _lastTempo.isPlaying && _hasLastPpq) {
      const double sampleRate = getSampleRate();
      const double expected =
          sampleRate > 0.0
              ? ((static_cast<double>(numSamples) * _lastTempo.bpm / 60.0) /
                 sampleRate)
              : 0.0;
      if (isPositionJump(_lastPpq, _lastTempo.ppqPosition, expected)) {
        _engine.reset();
        _engine.doQueuedActions();
        _midiOutput.allNotesOff(0);
      }
    }
    _lastPpq = _lastTempo.ppqPosition;
    _hasLastPpq = true;
  }
  _wasPlaying = _lastTempo.isPlaying;

  const bool run = !_lastTempo.hasHost || _lastTempo.isPlaying;

  // Effective tempo/root for the UI, published lock-free to the message thread.
  const float rootFreqParam = _apvts.getRawParameterValue("rootFreq")->load();
  const double effectiveBpm =
      _lastTempo.hasHost ? _lastTempo.bpm : rootFreqParam * 60.0;
  const double effectiveRoot =
      _lastTempo.isPlaying ? _lastTempo.bpm / 60.0 : rootFreqParam;
  _displayBpm.store(effectiveBpm);
  _displayRootFreq.store(effectiveRoot);

  // Mirror the message-thread mixer snapshot into the audio-thread MidiOutput.
  const auto& snapshot = _controller.getMixerSnapshot();
  for (int id = 0; id < NetworkState::kMaxNodes; ++id) {
    _midiOutput.setNodeNote(id, snapshot.note(id));
    _midiOutput.setNodeGain(id, snapshot.gain(id));
    _midiOutput.setNodeEnabled(id, snapshot.enabled(id));
  }

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
    const float quantAmount =
        _apvts.getRawParameterValue("masterQuantAmount")->load();

    _engine.setNodeFrequency(0, effectiveRoot, false);
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

double PluginProcessor::getRootFreq() const {
  return _displayRootFreq.load();
}

double PluginProcessor::getBpm() const {
  return _displayBpm.load();
}

// Phase 7: APVTS <-> controller automation facade (message thread only) -------
void PluginProcessor::parameterChanged(const juce::String& parameterID,
                                       float newValue) {
  if (_syncingParams)
    return;  // our own writeback, not host automation
  if (!parameterID.startsWith("node_"))
    return;

  const int id = parameterID.substring(5, 7).getIntValue();
  const juce::String field = parameterID.substring(8);

  _syncingParams = true;
  if (field == "volume")
    _controller.setNodeVolume(id, newValue);
  else if (field == "pitch")
    _controller.setNodeMidiNote(id, juce::roundToInt(newValue));
  else if (field == "mute")
    _controller.setNodeMuted(id, newValue > 0.5f);
  else if (field == "freq")
    _controller.setNodeFrequency(id, static_cast<double>(newValue), false);
  else if (field == "quantAmount")
    _controller.setNodeQuantiseAmount(id, newValue);
  _syncingParams = false;
}

void PluginProcessor::syncAllNodeParams() {
  if (_syncingParams)
    return;
  _syncingParams = true;
  for (int id = 0; id < NetworkState::kMaxNodes; ++id)
    syncNodeParams(id);
  _syncingParams = false;
}

void PluginProcessor::syncNodeParams(int nodeId) {
  const juce::ValueTree node =
      NetworkState::getNode(_controller.getTree(), nodeId);
  const bool exists = node.isValid();
  auto& cfg = ConfigManager::get();

  auto setValue = [this, nodeId](const char* field, float value) {
    if (auto* p = _apvts.getParameter(nodeParamId(nodeId, field)))
      p->setValueNotifyingHost(p->convertTo0to1(value));
  };

  if (nodeId != NetworkState::kRootNodeId)
    setValue("freq",
             exists ? static_cast<float>(_controller.getNodeFrequency(nodeId))
                    : cfg.newNodeFreqMultiple);
  setValue("volume", exists ? _controller.getNodeVolume(nodeId) : 0.0f);
  setValue("pitch",
           static_cast<float>(exists ? _controller.getNodeMidiNote(nodeId)
                                     : cfg.midiTriggerNote + nodeId));
  setValue("mute", exists && _controller.getNodeMuted(nodeId) ? 1.0f : 0.0f);
  setValue("quantAmount",
           exists ? _controller.getNodeQuantiseAmount(nodeId) : 1.0f);
}

void PluginProcessor::applyHostGrid() {
  // Called every frame: the controller skips nodes that already match, so this
  // also picks up a node that was just switched to "follow host".
  _controller.applyHostGrid(mapTimeSignatureToGrid(
      _hostTimeSigNumerator.load(), _hostTimeSigDenominator.load()));
}

}  // namespace neurythmic

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new neurythmic::PluginProcessor();
}
