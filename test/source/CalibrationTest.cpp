#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <vector>

#include "MatsuokaEngine.h"
#include "Neurythmic/PluginProcessor.h"

namespace {

std::vector<int> risingZeroCrossings(MatsuokaEngine& engine, int samples) {
  std::vector<int> crossings;
  auto previous = engine.getNode(0).getSignalState();
  for (int i = 0; i < samples; ++i) {
    engine.step();
    const auto state = engine.getNode(0).getSignalState();
    if (state == MatsuNode::signalState::zeroXup &&
        previous != MatsuNode::signalState::zeroXup) {
      crossings.push_back(i);
    }
    previous = state;
  }
  return crossings;
}

double measureRootHz(MatsuokaEngine& engine, int seconds) {
  const int samples = static_cast<int>(engine.getSampleRate()) * seconds;
  const auto crossings = risingZeroCrossings(engine, samples);
  if (crossings.size() < 2)
    return 0.0;
  const int intervals = static_cast<int>(crossings.size()) - 1;
  const int span = crossings.back() - crossings.front();
  return static_cast<double>(intervals) * engine.getSampleRate() / span;
}

}  // namespace

TEST(Calibration, RootFrequencyAtHostRates) {
  struct Case {
    double sampleRate;
    const char* label;
  };
  const Case cases[] = {{44100.0, "44.1 kHz"}, {48000.0, "48 kHz"},
                        {96000.0, "96 kHz"}};

  for (const auto& c : cases) {
    neurythmic::PluginProcessor processor;
    processor.setRateAndBufferSizeDetails(c.sampleRate, 512);
    processor.prepareToPlay(c.sampleRate, 512);

    auto& engine = processor.getEngine();
    engine.setNodeFrequency(0, 2.0, false);
    engine.doQueuedActions();

    const double measured = measureRootHz(engine, 8);
    std::cout << c.label << " measured root: " << measured << " Hz\n";
    EXPECT_NEAR(measured, 2.0, 0.02) << c.label;
  }
}

TEST(Calibration, SampleRateChangeReappliesRootFrequency) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource tempo;  // standalone: hasHost = false
  processor.setTempoSource(&tempo);
  processor.setRateAndBufferSizeDetails(44100.0, 512);
  processor.prepareToPlay(44100.0, 512);

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);
  EXPECT_NEAR(processor.getEngine().getNodeFrequency(0), 2.0, 1e-4);

  processor.setRateAndBufferSizeDetails(48000.0, 512);
  processor.prepareToPlay(48000.0, 512);
  processor.processBlock(buffer, midi);

  // Without re-applying the root at the new rate, t1 remains calibrated for
  // 44.1 kHz and getNodeFrequency(0) reports 2.0 * 48/44.1.
  EXPECT_NEAR(processor.getEngine().getNodeFrequency(0), 2.0, 1e-4);
}
