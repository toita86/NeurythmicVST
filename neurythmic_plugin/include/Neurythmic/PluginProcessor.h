#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MatsuokaEngine.h"

namespace neurythmic {

class PluginProcessor : public juce::AudioProcessor {
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

  std::array<bool, 16> popFiredNodes();
  // Getters exposed for the editor
  int getNodeCount() const;
  int getNodeSignalState(u_int id) const;
  double getNodeOutput(u_int id) const;
  double getNodeFrequency(u_int id) const;
  bool isEngineRunning() const;
  void startEngine();
  void stopEngine();
  const MatsuokaEngine& getEngine() const { return _engine; }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)

  MatsuokaEngine _engine;
  bool _running = true;
  std::array<bool, 16> _nodeFired{};

  void _setupNetwork();
};

}  // namespace neurythmic
