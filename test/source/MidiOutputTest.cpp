#include <gtest/gtest.h>

#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "MatsuokaEngine.h"
#include "Neurythmic/Neurythmic.h"

namespace {

struct Ev {
  int sample;
  int channel;
  int note;
  bool isOn;
  int velocity;
};

std::vector<Ev> events(const juce::MidiBuffer& buffer) {
  std::vector<Ev> out;
  for (const auto metadata : buffer) {
    const auto msg = metadata.getMessage();
    if (msg.isNoteOn())
      out.push_back({metadata.samplePosition, msg.getChannel(),
                     msg.getNoteNumber(), true, msg.getVelocity()});
    else if (msg.isNoteOff())
      out.push_back({metadata.samplePosition, msg.getChannel(),
                     msg.getNoteNumber(), false, msg.getVelocity()});
  }
  return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// MidiOutput: routing, retrigger, velocity, allNotesOff
// ---------------------------------------------------------------------------

TEST(MidiOutput, DrumModeRoutesSingleChannelDistinctNotes) {
  neurythmic::MidiOutput out(60, 10);

  out.fire(0, 1.0f, 0);
  out.fire(3, 1.0f, 10);

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 2u);
  EXPECT_EQ(evs[0].channel, 10);
  EXPECT_EQ(evs[0].note, 60);
  EXPECT_EQ(evs[1].channel, 10);
  EXPECT_EQ(evs[1].note, 63);
}

TEST(MidiOutput, PerChannelModeRoutesChannelById) {
  neurythmic::MidiOutput out(60, 10);
  out.setRoutingMode(neurythmic::RoutingMode::PerChannel);

  out.fire(0, 1.0f, 0);
  out.fire(3, 1.0f, 5);

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 2u);
  EXPECT_EQ(evs[0].channel, 1);
  EXPECT_EQ(evs[0].note, 60);
  EXPECT_EQ(evs[1].channel, 4);
  EXPECT_EQ(evs[1].note, 60);
}

TEST(MidiOutput, RetriggerEmitsNoteOffThenNoteOn) {
  neurythmic::MidiOutput out(60, 10);

  out.fire(0, 1.0f, 0);
  out.fire(0, 1.0f, 100);

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 3u);  // NoteOn(0), NoteOff(100), NoteOn(100)
  EXPECT_TRUE(evs[0].isOn);
  EXPECT_FALSE(evs[1].isOn);
  EXPECT_TRUE(evs[2].isOn);
  EXPECT_EQ(evs[1].sample, 100);
  EXPECT_EQ(evs[2].sample, 100);
}

TEST(MidiOutput, ConstantVelocityIgnoresAmplitude) {
  neurythmic::MidiOutput out(60, 10);
  out.setVelocityMode(neurythmic::VelocityMode::Constant);
  out.setMasterVolume(1.0f);

  out.fire(0, 0.1f, 0);
  out.fire(1, 2.0f, 1);

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 2u);
  EXPECT_EQ(evs[0].velocity, 127);
  EXPECT_EQ(evs[1].velocity, 127);
}

TEST(MidiOutput, AmplitudeVelocityScalesByCurve) {
  neurythmic::MidiOutput out(60, 10);
  out.setVelocityMode(neurythmic::VelocityMode::Amplitude);
  out.setMasterVolume(1.0f);

  out.fire(0, 0.5f, 0);   // (0.5*2)^2 = 1.0 -> 127
  out.fire(1, 0.25f, 1);  // (0.5)^2 = 0.25 -> 31

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 2u);
  EXPECT_EQ(evs[0].velocity, 127);
  EXPECT_EQ(evs[1].velocity, 31);
}

TEST(MidiOutput, VelocityClampsToMax) {
  neurythmic::MidiOutput out(60, 10);
  out.setVelocityMode(neurythmic::VelocityMode::Amplitude);
  out.setMasterVolume(1.0f);

  out.fire(0, 5.0f, 0);  // (10)^2 = 100 -> clamped to 1 -> 127

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 1u);
  EXPECT_EQ(evs[0].velocity, 127);
}

TEST(MidiOutput, MasterVolumeScalesVelocity) {
  neurythmic::MidiOutput out(60, 10);
  out.setVelocityMode(neurythmic::VelocityMode::Constant);
  out.setMasterVolume(0.5f);

  out.fire(0, 0.0f, 0);  // constant ignores amplitude -> 0.5 * 127 = 63

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 1u);
  EXPECT_EQ(evs[0].velocity, 63);
}

TEST(MidiOutput, AllNotesOffClearsHeldNotes) {
  neurythmic::MidiOutput out(60, 10);

  out.fire(0, 1.0f, 0);
  out.fire(2, 1.0f, 0);
  out.allNotesOff(50);

  juce::MidiBuffer buffer;
  out.flush(buffer);

  auto evs = events(buffer);
  ASSERT_EQ(evs.size(), 4u);  // 2 NoteOn + 2 NoteOff
  int offCount = 0;
  for (const auto& e : evs) {
    if (!e.isOn) {
      ++offCount;
      EXPECT_EQ(e.sample, 50);
    }
  }
  EXPECT_EQ(offCount, 2);
}

TEST(MidiOutput, ResetClearsPendingAndHeld) {
  neurythmic::MidiOutput out(60, 10);

  out.fire(0, 1.0f, 0);
  out.reset();
  out.allNotesOff(0);

  juce::MidiBuffer buffer;
  out.flush(buffer);

  EXPECT_TRUE(events(buffer).empty());
}

// ---------------------------------------------------------------------------
// processBlock integration via the TempoSource seam
// ---------------------------------------------------------------------------

TEST(ProcessBlockMidi, RootFreqDrivesEngineWhenNoHost) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = false;
  fake.info.isPlaying = false;
  processor.setTempoSource(&fake);

  *processor.getAPVTS().getRawParameterValue("rootFreq") = 1.0f;

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);

  EXPECT_NEAR(processor.getNodeFrequency(0), 1.0, 0.05);
}

TEST(ProcessBlockMidi, HostTempoDrivesEngineWhenPlaying) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = true;
  fake.info.isPlaying = true;
  fake.info.bpm = 90.0;
  processor.setTempoSource(&fake);

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);

  EXPECT_NEAR(processor.getNodeFrequency(0), 1.5, 0.05);  // 90 / 60
}

TEST(ProcessBlockMidi, ProducesNoteOnAfterEnoughSamples) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = false;  // fallback clock, engine runs at rootFreq
  processor.setTempoSource(&fake);

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer all;
  for (int i = 0; i < 300; ++i) {
    juce::MidiBuffer block;
    processor.processBlock(buffer, block);
    all.addEvents(block, 0, -1, i * 512);
  }

  bool foundNoteOn = false;
  for (const auto m : all) {
    if (m.getMessage().isNoteOn()) {
      foundNoteOn = true;
      break;
    }
  }
  EXPECT_TRUE(foundNoteOn);
}

TEST(ProcessBlockMidi, StopPausesEngineNoNewNotes) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource fake;
  fake.info.hasHost = true;
  fake.info.isPlaying = true;
  fake.info.bpm = 120.0;
  processor.setTempoSource(&fake);

  juce::AudioBuffer<float> buffer(2, 512);

  bool sawNoteOn = false;
  for (int i = 0; i < 300 && !sawNoteOn; ++i) {
    juce::MidiBuffer block;
    processor.processBlock(buffer, block);
    for (const auto m : block) {
      if (m.getMessage().isNoteOn()) {
        sawNoteOn = true;
        break;
      }
    }
  }
  ASSERT_TRUE(sawNoteOn);

  fake.info.isPlaying = false;
  juce::MidiBuffer stopBlock;
  processor.processBlock(buffer, stopBlock);

  juce::MidiBuffer afterBlock;
  processor.processBlock(buffer, afterBlock);

  for (const auto m : afterBlock) {
    EXPECT_FALSE(m.getMessage().isNoteOn());
  }
}

// ---------------------------------------------------------------------------
// State persistence
// ---------------------------------------------------------------------------

TEST(StatePersistence, RoundTripPreservesNetwork) {
  neurythmic::PluginProcessor a;
  a.getController().createChild(0);  // node 1
  a.getController().createChild(0);  // node 2

  juce::MemoryBlock mb;
  a.getStateInformation(mb);
  EXPECT_GT(mb.getSize(), 0u);

  neurythmic::PluginProcessor b;
  b.setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));

  EXPECT_EQ(b.getController().getNodeCount(), 3);
  EXPECT_TRUE(b.getEngine().nodeExists(1));
  EXPECT_TRUE(b.getEngine().nodeExists(2));
}
