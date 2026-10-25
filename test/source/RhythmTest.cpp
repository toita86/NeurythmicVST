#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include "MatsuokaEngine.h"
#include "QuantiseGrid_soft.h"
#include "QuantisedEventQueue.h"
#include "Neurythmic/Neurythmic.h"

namespace {

using Grid = MatsuokaEngine::gridType;

struct TimedEvent {
  int sample;
  int note;

  bool operator==(const TimedEvent& other) const {
    return sample == other.sample && note == other.note;
  }
};

std::vector<TimedEvent> collectNoteOns(neurythmic::PluginProcessor& processor,
                                       neurythmic::FakeTempoSource& tempo,
                                       juce::AudioBuffer<float>& buffer,
                                       int blocks,
                                       int blockSize) {
  std::vector<TimedEvent> events;
  for (int block = 0; block < blocks; ++block) {
    tempo.info.ppqPosition +=
        (static_cast<double>(blockSize) * tempo.info.bpm / 60.0) / 44100.0;
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    for (const auto metadata : midi) {
      const auto message = metadata.getMessage();
      if (message.isNoteOn())
        events.push_back(
            {block * blockSize + metadata.samplePosition, message.getNoteNumber()});
    }
  }
  return events;
}

std::unique_ptr<neurythmic::PluginProcessor> makeProcessor(
    neurythmic::FakeTempoSource& tempo) {
  auto processor = std::make_unique<neurythmic::PluginProcessor>();
  tempo.info.hasHost = true;
  tempo.info.isPlaying = true;
  tempo.info.bpm = 120.0;
  tempo.info.hasPpq = true;
  tempo.info.ppqPosition = 0.0;
  tempo.info.timeSigNumerator = 4;
  tempo.info.timeSigDenominator = 4;
  processor->setTempoSource(&tempo);
  processor->setRateAndBufferSizeDetails(44100.0, 512);
  processor->prepareToPlay(44100.0, 512);
  return processor;
}

}  // namespace

// ---------------------------------------------------------------------------
// C1 — time-signature -> 24/32 step bar mapping
// ---------------------------------------------------------------------------

TEST(TimeSignatureMapping, NumeratorAware) {
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(3, 4), Grid::_24th);
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(4, 4), Grid::_32nd);
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(6, 8), Grid::_24th);
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(2, 4), Grid::_24th);  // 16 -> 24
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(5, 4), Grid::_32nd);  // 40 -> 32
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(7, 8), Grid::_32nd);  // 28 tie
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(2, 2), Grid::_32nd);  // 32
}

TEST(TimeSignatureMapping, BeatsPerBarUsesQuarterNotes) {
  EXPECT_NEAR(neurythmic::beatsPerBarForSignature(4, 4), 4.0, 1e-9);
  EXPECT_NEAR(neurythmic::beatsPerBarForSignature(3, 4), 3.0, 1e-9);
  EXPECT_NEAR(neurythmic::beatsPerBarForSignature(6, 8), 3.0, 1e-9);
  EXPECT_NEAR(neurythmic::beatsPerBarForSignature(2, 2), 4.0, 1e-9);
}

// ---------------------------------------------------------------------------
// A1 — quantiser tempo is bars-per-minute, independent of node 0
// ---------------------------------------------------------------------------

TEST(QuantiserTempo, OverrideSurvivesNodeFrequencyUpdates) {
  MatsuokaEngine engine(44100);

  engine.setQuantiserTempo(30.0f);
  engine.doQueuedActions();
  EXPECT_FLOAT_EQ(engine.getQuantiserTempo(), 30.0f);

  engine.setNodeFrequency(0, 2.0, false);
  engine.doQueuedActions();
  EXPECT_FLOAT_EQ(engine.getQuantiserTempo(), 30.0f);

  engine.clearQuantiserTempoOverride();
  engine.doQueuedActions();
  EXPECT_FLOAT_EQ(engine.getQuantiserTempo(),
                  60.0f * static_cast<float>(engine.getNodeFrequency(0)));
}

TEST(QuantiserTempo, DrivesGridAtBarsPerMinute) {
  MatsuokaEngine engine(44100);
  engine.setQuantiserTempo(30.0f);  // 30 bars/min, 32 steps/bar = 16 steps/s
  engine.doQueuedActions();

  for (int i = 0; i < 44100; ++i)
    engine.step();

  EXPECT_NEAR(static_cast<double>(engine.getQuantiserGridMarker(Grid::_32nd)),
              16.0, 1.0);
}

TEST(QuantiserTempo, DelayFollowsBeatTempoNotGridTempo) {
  MatsuokaEngine engine(44100);

  engine.setQuantiserNoteDelayTempo(120.0f);
  engine.doQueuedActions();
  const auto expectedDelay = static_cast<unsigned>((44100.0 * 60.0) / (120.0 * 4.0));
  EXPECT_EQ(engine.getQuantiserNoteDelay(), expectedDelay);

  engine.setQuantiserTempo(30.0f);
  engine.doQueuedActions();
  EXPECT_EQ(engine.getQuantiserNoteDelay(), expectedDelay);
}

// ---------------------------------------------------------------------------
// B3a — deterministic grid seeding from ppq/bars
// ---------------------------------------------------------------------------

TEST(QuantiserSeek, SeedsBothGridsFromBars) {
  MatsuokaEngine engine(44100);

  engine.seekQuantiserToBars(1.25);  // one bar + one beat in 4/4
  engine.doQueuedActions();
  EXPECT_EQ(engine.getQuantiserGridMarker(Grid::_32nd), 40u);  // 1.25 * 32
  EXPECT_NEAR(engine.getQuantiserGridPhase(Grid::_32nd), 0.0f, 1e-6f);
  EXPECT_EQ(engine.getQuantiserGridMarker(Grid::_24th), 30u);  // 1.25 * 24
  EXPECT_NEAR(engine.getQuantiserGridPhase(Grid::_24th), 0.0f, 1e-6f);

  engine.seekQuantiserToBars(1.26);
  engine.doQueuedActions();
  EXPECT_EQ(engine.getQuantiserGridMarker(Grid::_32nd), 40u);
  EXPECT_NEAR(engine.getQuantiserGridPhase(Grid::_32nd), 0.32f, 1e-4f);
  EXPECT_EQ(engine.getQuantiserGridMarker(Grid::_24th), 30u);
  EXPECT_NEAR(engine.getQuantiserGridPhase(Grid::_24th), 0.24f, 1e-4f);
}

TEST(QuantiserSeek, HugePositionIsSeededDirectly) {
  MatsuokaEngine engine(44100);

  engine.seekQuantiserToBars(10000.0);
  engine.doQueuedActions();
  EXPECT_EQ(engine.getQuantiserGridMarker(Grid::_32nd), 320000u);
  EXPECT_EQ(engine.getQuantiserGridMarker(Grid::_24th), 240000u);
}

TEST(QuantiserSeek, ClearDropsInFlightEvents) {
  QuantisedEventQueue queue(44100, 120.0f);
  queue.addEvent({1, 1.0});
  queue.clearEvents();

  for (int i = 0; i < 2000; ++i)
    queue.tick();

  const auto event = queue.getNote();
  EXPECT_EQ(event.nodeID, static_cast<unsigned>(-1));
}

// ---------------------------------------------------------------------------
// Transport determinism through processBlock
// ---------------------------------------------------------------------------

TEST(TransportDeterminism, RestartAfterResetReproducesEvents) {
  constexpr int blockSize = 512;
  constexpr int blocks = 200;
  juce::AudioBuffer<float> buffer(2, blockSize);

  // Baseline: a freshly started processor.
  neurythmic::FakeTempoSource firstTempo;
  auto first = makeProcessor(firstTempo);
  first->getController().createChild(0);
  first->getController().createChild(0);
  *first->getAPVTS().getRawParameterValue("masterQuantAmount") = 1.0f;
  first->applyHostGrid();
  const auto expected = collectNoteOns(*first, firstTempo, buffer, blocks, blockSize);
  ASSERT_FALSE(expected.empty());

  // Play, stop, reset the playhead to zero, then play again.
  neurythmic::FakeTempoSource secondTempo;
  auto second = makeProcessor(secondTempo);
  second->getController().createChild(0);
  second->getController().createChild(0);
  *second->getAPVTS().getRawParameterValue("masterQuantAmount") = 1.0f;
  second->applyHostGrid();

  juce::MidiBuffer warmup;
  for (int i = 0; i < 40; ++i) {
    secondTempo.info.ppqPosition +=
        (static_cast<double>(blockSize) * secondTempo.info.bpm / 60.0) / 44100.0;
    second->processBlock(buffer, warmup);
  }

  secondTempo.info.isPlaying = false;
  juce::MidiBuffer stopped;
  second->processBlock(buffer, stopped);

  secondTempo.info.ppqPosition = 0.0;
  secondTempo.info.isPlaying = true;

  const auto actual = collectNoteOns(*second, secondTempo, buffer, blocks, blockSize);

  ASSERT_EQ(expected.size(), actual.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(expected[i].sample, actual[i].sample) << "event " << i;
    EXPECT_EQ(expected[i].note, actual[i].note) << "event " << i;
  }
}

TEST(TransportDeterminism, PurePauseDoesNotResync) {
  constexpr int blockSize = 512;
  juce::AudioBuffer<float> buffer(2, blockSize);

  neurythmic::FakeTempoSource tempo;
  auto processor = makeProcessor(tempo);

  juce::MidiBuffer midi;
  for (int i = 0; i < 20; ++i) {
    tempo.info.ppqPosition +=
        (static_cast<double>(blockSize) * tempo.info.bpm / 60.0) / 44100.0;
    processor->processBlock(buffer, midi);
  }

  const int before = processor->getTransportResyncCount();

  tempo.info.isPlaying = false;
  processor->processBlock(buffer, midi);
  tempo.info.isPlaying = true;
  tempo.info.ppqPosition +=
      (static_cast<double>(blockSize) * tempo.info.bpm / 60.0) / 44100.0;
  processor->processBlock(buffer, midi);

  EXPECT_EQ(processor->getTransportResyncCount(), before);
}

TEST(TransportDeterminism, ResetWhileStoppedResyncsOnPlay) {
  constexpr int blockSize = 512;
  juce::AudioBuffer<float> buffer(2, blockSize);

  neurythmic::FakeTempoSource tempo;
  auto processor = makeProcessor(tempo);

  juce::MidiBuffer midi;
  for (int i = 0; i < 20; ++i) {
    tempo.info.ppqPosition +=
        (static_cast<double>(blockSize) * tempo.info.bpm / 60.0) / 44100.0;
    processor->processBlock(buffer, midi);
  }

  const int before = processor->getTransportResyncCount();

  tempo.info.isPlaying = false;
  processor->processBlock(buffer, midi);

  tempo.info.ppqPosition = 0.0;
  tempo.info.isPlaying = true;
  processor->processBlock(buffer, midi);

  EXPECT_GT(processor->getTransportResyncCount(), before);
}

// ---------------------------------------------------------------------------
// Freedom: zero quantisation amount leaves the raw coordinate untouched
// ---------------------------------------------------------------------------

TEST(QuantiserFreedom, ZeroAmountKeepsRawCoordinate) {
  QuantiseGrid_soft grid(0, 32, 44100, 120.0f);
  for (int i = 0; i < 137; ++i)
    grid.tick();

  grid.setQuantiseAmount(0.0f);
  const auto raw = grid.getNoteCoordinateUnquantised();
  const auto free = grid.getNoteCoordinate(4, 1);
  EXPECT_EQ(free.gridline, raw.gridline);
  EXPECT_NEAR(free.phase, raw.phase, 1e-5f);
}

TEST(QuantiserFreedom, FullAmountSnapsToGrid) {
  QuantiseGrid_soft grid(0, 32, 44100, 120.0f);
  for (int i = 0; i < 137; ++i)
    grid.tick();

  grid.setQuantiseAmount(1.0f);
  const auto snapped = grid.getNoteCoordinate(4, 1);
  EXPECT_EQ(snapped.gridline % 4, 1u);
  EXPECT_NEAR(snapped.phase, 0.0f, 1e-5f);
}
