#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Neurythmic/Neurythmic.h"

namespace {

float normalised(juce::RangedAudioParameter& p, float value) {
  return p.convertTo0to1(value);
}

}  // namespace

// ---------------------------------------------------------------------------
// The fixed 16-slot APVTS parameter pool
// ---------------------------------------------------------------------------

TEST(AutomationPool, NodeParamsExist) {
  neurythmic::PluginProcessor p;
  auto& apvts = p.getAPVTS();

  EXPECT_NE(apvts.getParameter("node_00_volume"), nullptr);
  EXPECT_NE(apvts.getParameter("node_00_pitch"), nullptr);
  EXPECT_NE(apvts.getParameter("node_00_mute"), nullptr);
  EXPECT_NE(apvts.getParameter("node_00_quantAmount"), nullptr);
  // Root frequency is the global rootFreq parameter, not a node param.
  EXPECT_EQ(apvts.getParameter("node_00_freq"), nullptr);

  EXPECT_NE(apvts.getParameter("node_01_freq"), nullptr);
  EXPECT_NE(apvts.getParameter("node_15_volume"), nullptr);
  EXPECT_NE(apvts.getParameter("node_15_freq"), nullptr);
}

// ---------------------------------------------------------------------------
// Host automation -> controller
// ---------------------------------------------------------------------------

TEST(AutomationBridge, HostWritesReachController) {
  neurythmic::PluginProcessor p;
  p.getController().createChild(0);  // node 1

  auto* param = p.getAPVTS().getParameter("node_01_volume");
  ASSERT_NE(param, nullptr);
  auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
  ASSERT_NE(ranged, nullptr);
  param->setValueNotifyingHost(normalised(*ranged, 0.42f));

  EXPECT_NEAR(p.getController().getNodeVolume(1), 0.42f, 1e-3f);
}

TEST(AutomationBridge, PitchAndMuteRoundTripFromHost) {
  neurythmic::PluginProcessor p;
  p.getController().createChild(0);

  auto* pitch = p.getAPVTS().getParameter("node_01_pitch");
  auto* pitchR = dynamic_cast<juce::RangedAudioParameter*>(pitch);
  ASSERT_NE(pitchR, nullptr);
  pitch->setValueNotifyingHost(normalised(*pitchR, 72.0f));
  EXPECT_EQ(p.getController().getNodeMidiNote(1), 72);

  auto* mute = p.getAPVTS().getParameter("node_01_mute");
  auto* muteR = dynamic_cast<juce::RangedAudioParameter*>(mute);
  ASSERT_NE(muteR, nullptr);
  mute->setValueNotifyingHost(normalised(*muteR, 1.0f));
  EXPECT_TRUE(p.getController().getNodeMuted(1));
}

// ---------------------------------------------------------------------------
// Controller -> host automation facade
// ---------------------------------------------------------------------------

TEST(AutomationBridge, ControllerWritesReachHost) {
  neurythmic::PluginProcessor p;
  int id = p.getController().createChild(0);
  p.getController().setNodeMuted(id, true);

  auto* mute = p.getAPVTS().getParameter(
      neurythmic::PluginProcessor::nodeParamId(id, "mute"));
  ASSERT_NE(mute, nullptr);
  EXPECT_GT(mute->getValue(), 0.5f);
}

TEST(AutomationBridge, NoFeedbackLoopOnControllerWrite) {
  neurythmic::PluginProcessor p;
  int id = p.getController().createChild(0);

  p.getController().setNodeVolume(id, 0.33f);

  EXPECT_NEAR(p.getController().getNodeVolume(id), 0.33f, 1e-4f);
  auto* vol = p.getAPVTS().getParameter(
      neurythmic::PluginProcessor::nodeParamId(id, "volume"));
  auto* volR = dynamic_cast<juce::RangedAudioParameter*>(vol);
  ASSERT_NE(volR, nullptr);
  EXPECT_NEAR(vol->getValue(), normalised(*volR, 0.33f), 1e-3f);
}

TEST(AutomationBridge, DeletedNodeSlotResets) {
  neurythmic::PluginProcessor p;
  int id = p.getController().createChild(0);
  p.getController().setNodeVolume(id, 1.2f);

  auto* vol = p.getAPVTS().getParameter(
      neurythmic::PluginProcessor::nodeParamId(id, "volume"));
  ASSERT_NE(vol, nullptr);
  EXPECT_GT(vol->getValue(), 0.5f);

  p.getController().deleteNode(id);

  // The slot becomes inert (volume 0) rather than removed.
  EXPECT_NEAR(vol->getValue(), 0.0f, 1e-3f);
}

// ---------------------------------------------------------------------------
// Preset round-trip keeps the facade in sync
// ---------------------------------------------------------------------------

TEST(AutomationBridge, PresetRoundTripPreservesNodeParams) {
  neurythmic::PluginProcessor a;
  int id = a.getController().createChild(0);
  a.getController().setNodeVolume(id, 0.9f);
  a.getController().setNodeMidiNote(id, 55);
  a.getController().setNodeMuted(id, true);
  a.getController().setNodeQuantiseAmount(id, 0.4f);

  juce::MemoryBlock mb;
  a.getStateInformation(mb);

  neurythmic::PluginProcessor b;
  b.setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));

  EXPECT_NEAR(b.getController().getNodeVolume(id), 0.9f, 1e-3f);
  EXPECT_EQ(b.getController().getNodeMidiNote(id), 55);
  EXPECT_TRUE(b.getController().getNodeMuted(id));
  EXPECT_NEAR(b.getController().getNodeQuantiseAmount(id), 0.4f, 1e-3f);

  auto* vol = b.getAPVTS().getParameter(
      neurythmic::PluginProcessor::nodeParamId(id, "volume"));
  auto* volR = dynamic_cast<juce::RangedAudioParameter*>(vol);
  ASSERT_NE(volR, nullptr);
  EXPECT_NEAR(vol->getValue(), normalised(*volR, 0.9f), 1e-3f);
}
