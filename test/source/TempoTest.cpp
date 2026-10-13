#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include "MatsuokaEngine.h"
#include "Neurythmic/Neurythmic.h"

// ---------------------------------------------------------------------------
// Time-signature -> CPGLib grid mapping
// ---------------------------------------------------------------------------

TEST(TempoMapping, MapsByDenominator) {
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(4, 4),
            MatsuokaEngine::gridType::_32nd);
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(3, 4),
            MatsuokaEngine::gridType::_32nd);
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(6, 8),
            MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(neurythmic::mapTimeSignatureToGrid(7, 8),
            MatsuokaEngine::gridType::_24th);
}

// ---------------------------------------------------------------------------
// ppq discontinuity predicate
// ---------------------------------------------------------------------------

TEST(PositionJump, DetectsBackwardJump) {
  EXPECT_TRUE(neurythmic::isPositionJump(8.0, 0.0, 0.02));
  EXPECT_TRUE(neurythmic::isPositionJump(4.0, 3.99, 0.02));
}

TEST(PositionJump, DetectsOvershoot) {
  // A jump far larger than a block (plus the half-beat tolerance).
  EXPECT_TRUE(neurythmic::isPositionJump(1.0, 100.0, 0.02));
}

TEST(PositionJump, AllowsNormalAdvance) {
  EXPECT_FALSE(neurythmic::isPositionJump(1.0, 1.02, 0.02));
  EXPECT_FALSE(neurythmic::isPositionJump(0.0, 0.02322, 0.02322));
}

TEST(PositionJump, AllowsSmallTolerance) {
  EXPECT_FALSE(neurythmic::isPositionJump(1.0, 1.3, 0.02));
  EXPECT_FALSE(neurythmic::isPositionJump(1.0, 1.52, 0.02));
}

// ---------------------------------------------------------------------------
// Host grid application at the controller level
// ---------------------------------------------------------------------------

TEST(HostGrid, AppliedToFollowHostNodes) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController ctrl(engine, neurythmic::ConfigManager::get());
  const int child = ctrl.createChild(0);

  EXPECT_TRUE(ctrl.getNodeQuantiseGridFollowHost(child));

  ctrl.applyHostGrid(MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(engine.getNodeQuantiser_Grid(child),
            MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(ctrl.getNodeQuantiseGrid(child), MatsuokaEngine::gridType::_24th);
}

TEST(HostGrid, ExplicitGridOptsOut) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController ctrl(engine, neurythmic::ConfigManager::get());
  const int child = ctrl.createChild(0);

  ctrl.setNodeQuantiseGrid(child, MatsuokaEngine::gridType::_32nd);
  EXPECT_FALSE(ctrl.getNodeQuantiseGridFollowHost(child));

  ctrl.applyHostGrid(MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(ctrl.getNodeQuantiseGrid(child), MatsuokaEngine::gridType::_32nd);
}

TEST(HostGrid, OptInAppliesCurrentHostGridImmediately) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController ctrl(engine, neurythmic::ConfigManager::get());
  const int child = ctrl.createChild(0);

  ctrl.applyHostGrid(MatsuokaEngine::gridType::_24th);   // establishes host grid
  ctrl.setNodeQuantiseGrid(child, MatsuokaEngine::gridType::_32nd);  // opt out
  ctrl.setNodeQuantiseGridFollowHost(child, true);       // opt back in

  EXPECT_TRUE(ctrl.getNodeQuantiseGridFollowHost(child));
  EXPECT_EQ(engine.getNodeQuantiser_Grid(child),
            MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(ctrl.getNodeQuantiseGrid(child), MatsuokaEngine::gridType::_24th);
}

// ---------------------------------------------------------------------------
// Mixer display tempo/root values
// ---------------------------------------------------------------------------

TEST(TransportDisplay, HostBpmShownEvenWhenStopped) {
  neurythmic::PluginProcessor p;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = true;
  fake.info.isPlaying = false;
  fake.info.bpm = 100.0;
  p.setTempoSource(&fake);
  *p.getAPVTS().getRawParameterValue("rootFreq") = 2.0f;

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  EXPECT_NEAR(p.getBpm(), 100.0, 1e-6);
  EXPECT_NEAR(p.getRootFreq(), 2.0, 1e-6);
}

TEST(TransportDisplay, PlayingRootFollowsTempo) {
  neurythmic::PluginProcessor p;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = true;
  fake.info.isPlaying = true;
  fake.info.bpm = 120.0;
  p.setTempoSource(&fake);

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  EXPECT_NEAR(p.getBpm(), 120.0, 1e-6);
  EXPECT_NEAR(p.getRootFreq(), 2.0, 1e-6);  // 120 / 60
}

TEST(TransportDisplay, NoHostFallsBackToRootFreq) {
  neurythmic::PluginProcessor p;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = false;
  p.setTempoSource(&fake);
  *p.getAPVTS().getRawParameterValue("rootFreq") = 1.5f;

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  p.processBlock(buffer, midi);

  EXPECT_NEAR(p.getBpm(), 90.0, 1e-6);  // 1.5 * 60
  EXPECT_NEAR(p.getRootFreq(), 1.5, 1e-6);
}

// ---------------------------------------------------------------------------
// Transport integration through the plugin's processBlock
// ---------------------------------------------------------------------------

TEST(TransportIntegration, BackwardPpqJumpReleasesHeldNotes) {
  neurythmic::PluginProcessor p;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = true;
  fake.info.isPlaying = true;
  fake.info.bpm = 120.0;
  fake.info.hasPpq = true;
  fake.info.ppqPosition = 0.0;
  p.setTempoSource(&fake);
  p.prepareToPlay(44100.0, 512);

  juce::AudioBuffer<float> buffer(2, 512);
  const double dt = 512.0 * 120.0 / 60.0 / 44100.0;

  bool sawNoteOn = false;
  for (int i = 0; i < 400 && !sawNoteOn; ++i) {
    fake.info.ppqPosition += dt;
    juce::MidiBuffer block;
    p.processBlock(buffer, block);
    for (const auto m : block)
      if (m.getMessage().isNoteOn())
        sawNoteOn = true;
  }
  ASSERT_TRUE(sawNoteOn);

  // Loop / seek backward by four beats.
  fake.info.ppqPosition -= 8.0;
  juce::MidiBuffer jump;
  p.processBlock(buffer, jump);

  bool sawNoteOff = false;
  for (const auto m : jump)
    if (m.getMessage().isNoteOff())
      sawNoteOff = true;
  EXPECT_TRUE(sawNoteOff);
}

TEST(TransportIntegration, RestartAfterStopDoesNotTreatPpqResetAsSeek) {
  neurythmic::PluginProcessor p;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = true;
  fake.info.isPlaying = true;
  fake.info.bpm = 120.0;
  fake.info.hasPpq = true;
  fake.info.ppqPosition = 4.0;
  p.setTempoSource(&fake);
  p.prepareToPlay(44100.0, 512);

  juce::AudioBuffer<float> buffer(2, 512);

  // Run for a while.
  for (int i = 0; i < 20; ++i) {
    fake.info.ppqPosition += 0.02322;
    juce::MidiBuffer block;
    p.processBlock(buffer, block);
  }

  // Stop, then restart from the beginning (ppq resets backwards).
  fake.info.isPlaying = false;
  juce::MidiBuffer stop;
  p.processBlock(buffer, stop);

  fake.info.isPlaying = true;
  fake.info.ppqPosition = 0.0;
  juce::MidiBuffer restart;
  p.processBlock(buffer, restart);

  // The stop->start transition must not be treated as a seek, so no all-notes
  // off NoteOff should be emitted on the restart block.
  for (const auto m : restart)
    EXPECT_FALSE(m.getMessage().isNoteOff());
}
