#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>
#include "MatsuokaEngine.h"
#include "MidiOutput.h"
#include "NetworkController.h"
#include "TempoSource.h"

namespace neurythmic {

class PluginProcessor : public juce::AudioProcessor,
                        private juce::AudioProcessorValueTreeState::Listener {
public:
  PluginProcessor();
  ~PluginProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  void processBlock(juce::AudioBuffer<double>& buffer,
                    juce::MidiBuffer& midiMessages) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  // Phase 2 API
  juce::AudioProcessorValueTreeState& getAPVTS() { return _apvts; }
  void setTempoSource(TempoSource* source);  // test seam
  bool isPlaying() const;                    // last known transport state

  // Phase 7 API
  // Builds the APVTS parameter id for a node field, e.g. ("volume", 0) ->
  // "node_00_volume". Zero-padded so the DAW list sorts naturally.
  static juce::String nodeParamId(int nodeId, const char* field);
  // Applies the host time signature to every "follow host" node. Message
  // thread (called from the editor timer); safe to call headlessly.
  void applyHostGrid();

  // Getters exposed for the editor
  double getNodeFrequency(unsigned id) const;
  double getRootFreq() const;  // effective root freq used by the engine (Hz)
  double getBpm() const;       // effective tempo shown in the mixer (BPM)
  NetworkController& getController() { return _controller; }
  const MatsuokaEngine& getEngine() const { return _engine; }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();
  void process(juce::MidiBuffer& midiMessages, int numSamples);

  // Phase 7 APVTS automation facade (message thread only).
  void parameterChanged(const juce::String& parameterID,
                        float newValue) override;
  void syncAllNodeParams();
  void syncNodeParams(int nodeId);

  MatsuokaEngine _engine;
  NetworkController _controller;  // MUST be declared after engine
  MidiOutput _midiOutput;
  juce::AudioProcessorValueTreeState _apvts;

  PlayheadTempoSource _defaultTempoSource;
  TempoSource* _tempoSource;
  TempoInfo _lastTempo;
  int _currentSample = 0;
  std::atomic<bool> _midiResetRequested{false};

  // Phase 7 bridge / transport state.
  bool _syncingParams = false;
  bool _wasPlaying = false;
  bool _hasLastPpq = false;
  double _lastPpq = 0.0;
  std::atomic<int> _hostTimeSigNumerator{4};
  std::atomic<int> _hostTimeSigDenominator{4};

  // Lock-free display values for the UI (written on the audio thread).
  std::atomic<double> _displayBpm{120.0};
  std::atomic<double> _displayRootFreq{2.0};
};

}  // namespace neurythmic
