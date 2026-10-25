#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include "MatsuokaEngine.h"
#include "Neurythmic/Neurythmic.h"

namespace {

using Grid = MatsuokaEngine::gridType;

struct TimedNote {
  int sample;
  int note;
};

std::vector<TimedNote> collectNoteOns(neurythmic::PluginProcessor& processor,
                                      neurythmic::FakeTempoSource& tempo,
                                      juce::AudioBuffer<float>& buffer,
                                      int blocks,
                                      int blockSize) {
  std::vector<TimedNote> notes;
  for (int block = 0; block < blocks; ++block) {
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    for (const auto metadata : midi) {
      const auto message = metadata.getMessage();
      if (message.isNoteOn())
        notes.push_back(
            {block * blockSize + metadata.samplePosition, message.getNoteNumber()});
    }
    tempo.info.ppqPosition +=
        (static_cast<double>(blockSize) * tempo.info.bpm / 60.0) / 44100.0;
  }
  return notes;
}

void prepare(neurythmic::PluginProcessor& processor,
             neurythmic::FakeTempoSource& tempo,
             double bpm = 120.0,
             int numerator = 4,
             int denominator = 4,
             double ppq = 0.0) {
  tempo.info.hasHost = true;
  tempo.info.isPlaying = true;
  tempo.info.bpm = bpm;
  tempo.info.hasPpq = true;
  tempo.info.ppqPosition = ppq;
  tempo.info.timeSigNumerator = numerator;
  tempo.info.timeSigDenominator = denominator;
  processor.setTempoSource(&tempo);
  processor.setRateAndBufferSizeDetails(44100.0, 512);
  processor.prepareToPlay(44100.0, 512);
}

}  // namespace

// ---------------------------------------------------------------------------
// C: the root runs at bars per second
// ---------------------------------------------------------------------------

TEST(RootBarTempo, RootFollowsBarLength) {
  struct Case {
    int numerator;
    int denominator;
    double expected;
  };
  const Case cases[] = {{4, 4, 0.5}, {3, 4, 120.0 / 180.0},
                        {6, 8, 120.0 / 180.0}};

  for (const auto& c : cases) {
    neurythmic::PluginProcessor processor;
    neurythmic::FakeTempoSource tempo;
    prepare(processor, tempo, 120.0, c.numerator, c.denominator);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    EXPECT_NEAR(processor.getRootFreq(), c.expected, 1e-6)
        << c.numerator << "/" << c.denominator;
  }
}

// ---------------------------------------------------------------------------
// Canonical frequency multiples
// ---------------------------------------------------------------------------

TEST(FrequencyMultiple, ChildStoresMultipleAndDerivesAbsolute) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int child = controller.createChild(0);
  const double defaultMultiple =
      neurythmic::ConfigManager::get().newNodeFreqMultiple;

  EXPECT_DOUBLE_EQ(controller.getNodeFrequencyMultiple(child), defaultMultiple);
  EXPECT_NEAR(controller.getNodeFrequency(child),
              engine.getNodeFrequency(0) * defaultMultiple, 1e-6);

  controller.setNodeFrequencyMultiple(child, 3.0);
  EXPECT_DOUBLE_EQ(controller.getNodeFrequencyMultiple(child), 3.0);
  EXPECT_NEAR(controller.getNodeFrequency(child),
              engine.getNodeFrequency(0) * 3.0, 1e-6);
}

TEST(FrequencyMultiple, RatioPreservedAcrossTempoChanges) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource tempo;
  prepare(processor, tempo);

  const int child = processor.getController().createChild(0);
  processor.getController().setNodeFrequencyMultiple(child, 2.0);

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);

  const double ratioAt120 =
      processor.getEngine().getNodeFrequency(child) /
      processor.getEngine().getNodeFrequency(0);
  EXPECT_NEAR(ratioAt120, 2.0, 0.01);

  tempo.info.bpm = 90.0;
  processor.processBlock(buffer, midi);

  const double ratioAt90 =
      processor.getEngine().getNodeFrequency(child) /
      processor.getEngine().getNodeFrequency(0);
  EXPECT_NEAR(ratioAt90, 2.0, 0.01);
  EXPECT_DOUBLE_EQ(processor.getController().getNodeFrequencyMultiple(child),
                   2.0);
}

TEST(FrequencyMultiple, PresetRoundTripPreservesMultiple) {
  neurythmic::PluginProcessor a;
  const int child = a.getController().createChild(0);
  a.getController().setNodeFrequencyMultiple(child, 3.5);

  juce::MemoryBlock mb;
  a.getStateInformation(mb);

  neurythmic::PluginProcessor b;
  b.setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));

  EXPECT_DOUBLE_EQ(b.getController().getNodeFrequencyMultiple(child), 3.5);
  EXPECT_EQ(static_cast<int>(b.getController().getTree().getProperty(
                neurythmic::NetworkState::Props::stateVersion)),
            neurythmic::NetworkState::kStateVersion);
}

TEST(FrequencyMultiple, MigratesVersion1AbsoluteValues) {
  neurythmic::PluginProcessor processor;

  auto tree = neurythmic::NetworkState::initEmptyNetwork();
  tree.setProperty(neurythmic::NetworkState::Props::stateVersion, 1, nullptr);
  auto root = neurythmic::NetworkState::getNode(tree, 0);
  root.setProperty(neurythmic::NetworkState::Props::freq, 2.0, nullptr);
  auto child = neurythmic::NetworkState::createNode(tree, 1, 0, 0.5f, 0.5f);
  child.setProperty(neurythmic::NetworkState::Props::freq, 4.0, nullptr);

  processor.getController().rebuild(tree);

  EXPECT_NEAR(processor.getController().getNodeFrequencyMultiple(1), 2.0,
              1e-6);
  EXPECT_EQ(static_cast<int>(processor.getController().getTree().getProperty(
                neurythmic::NetworkState::Props::stateVersion)),
            neurythmic::NetworkState::kStateVersion);
}

// ---------------------------------------------------------------------------
// Hard bar sync
// ---------------------------------------------------------------------------

TEST(RootBarSync, RootNoteLandsOnBarBoundaries) {
  constexpr int blockSize = 512;
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource tempo;
  prepare(processor, tempo);
  *processor.getAPVTS().getRawParameterValue("masterQuantAmount") = 1.0f;
  processor.applyHostGrid();

  juce::AudioBuffer<float> buffer(2, blockSize);
  // 120 BPM 4/4: one bar = 2 s = 88200 samples.
  const auto notes = collectNoteOns(processor, tempo, buffer, 400, blockSize);

  std::vector<int> rootSamples;
  for (const auto& note : notes)
    if (note.note == 60)
      rootSamples.push_back(note.sample);

  ASSERT_GE(rootSamples.size(), 2u);
  EXPECT_NEAR(rootSamples[0], 0, 2);
  EXPECT_NEAR(rootSamples[1], 88200, 2);
}

TEST(RootBarSync, MidBarStartSuppressesRootUntilNextBar) {
  constexpr int blockSize = 512;
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource tempo;
  prepare(processor, tempo, 120.0, 4, 4, 1.0);  // beat 2 of bar 0
  *processor.getAPVTS().getRawParameterValue("masterQuantAmount") = 1.0f;
  processor.applyHostGrid();

  juce::AudioBuffer<float> buffer(2, blockSize);
  const auto notes = collectNoteOns(processor, tempo, buffer, 400, blockSize);

  for (const auto& note : notes) {
    if (note.note == 60)
      EXPECT_GE(note.sample, 66150);  // next bar boundary (0.75 * 88200)
  }
}

// ---------------------------------------------------------------------------
// Meter changes
// ---------------------------------------------------------------------------

TEST(MeterChange, ReseedsGridAndTempo) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource tempo;
  prepare(processor, tempo);

  juce::AudioBuffer<float> buffer(2, 1);
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);

  // Change to 3/4 at ppq 4: the new bar position is 4 / 3.
  tempo.info.timeSigNumerator = 3;
  tempo.info.timeSigDenominator = 4;
  tempo.info.ppqPosition = 4.0;
  processor.processBlock(buffer, midi);

  EXPECT_NEAR(processor.getEngine().getQuantiserTempo(), 40.0, 0.01);
  EXPECT_EQ(processor.getEngine().getQuantiserGridMarker(Grid::_32nd), 42u);
  EXPECT_NEAR(processor.getEngine().getQuantiserGridPhase(Grid::_32nd), 0.6667f,
              1e-3f);
}

// ---------------------------------------------------------------------------
// Reset determinism
// ---------------------------------------------------------------------------

TEST(RootReset, FirstEventTimingIsHistoryIndependent) {
  auto firstEventOffset = [](int warmupSteps) {
    MatsuokaEngine engine(44100);
    int eventStep = -1;
    engine.setEventCallback([&](int nodeID, float) {
      if (nodeID == 0 && eventStep < 0)
        eventStep = static_cast<int>(engine.getEngineStepCounter());
    });

    for (int i = 0; i < warmupSteps; ++i)
      engine.step();

    engine.reset();
    engine.seekQuantiserToBars(0.0);
    engine.setNodeFrequency(0, engine.getNodeFrequency(0), false);
    engine.setQuantiserNoteDelayTempo(
        static_cast<float>(engine.getNodeFrequency(0) * 60.0));
    engine.doQueuedActions();
    const int startStep = static_cast<int>(engine.getEngineStepCounter());

    for (int i = 0; i < 200000 && eventStep < 0; ++i)
      engine.step();

    return eventStep < 0 ? -1 : eventStep - startStep;
  };

  const int first = firstEventOffset(100);
  const int second = firstEventOffset(5000);

  ASSERT_GE(first, 0);
  EXPECT_EQ(first, second);
}
