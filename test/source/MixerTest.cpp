#include <gtest/gtest.h>

#include "MatsuokaEngine.h"
#include "Neurythmic/ConfigManager.h"
#include "Neurythmic/MenuValues.h"
#include "Neurythmic/MixerSnapshot.h"
#include "Neurythmic/NetworkController.h"
#include "Neurythmic/PluginProcessor.h"

namespace {

using neurythmic::ConfigManager;
using neurythmic::NetworkController;

struct Fixture {
  MatsuokaEngine engine{44100};
  NetworkController ctrl{engine, ConfigManager::get()};
};

}  // namespace

// ---------------------------------------------------------------------------
// Per-node mixer state: defaults, round-trips, snapshot mirror
// ---------------------------------------------------------------------------

TEST(Mixer, VolumeDefaultsToConfigInit) {
  Fixture f;
  f.ctrl.createChild(0);  // node 1
  EXPECT_FLOAT_EQ(f.ctrl.getNodeVolume(1),
                  ConfigManager::get().newNodeVolumeInit);
}

TEST(Mixer, SetNodeVolumeRoundTripsAndMirrorsSnapshot) {
  Fixture f;
  f.ctrl.setNodeVolume(0, 0.33f);
  EXPECT_FLOAT_EQ(f.ctrl.getNodeVolume(0), 0.33f);
  EXPECT_NEAR(f.ctrl.getMixerSnapshot().gain(0), 0.33f, 1e-6);
}

TEST(Mixer, MidiNoteDefaultsToBasePlusId) {
  Fixture f;
  f.ctrl.createChild(0);  // node 1
  EXPECT_EQ(f.ctrl.getNodeMidiNote(2), -1);  // node 2 does not exist yet
  EXPECT_EQ(f.ctrl.getNodeMidiNote(1),
            ConfigManager::get().midiTriggerNote + 1);
}

TEST(Mixer, SetNodeMidiNoteRoundTripsAndMirrorsSnapshot) {
  Fixture f;
  f.ctrl.setNodeMidiNote(0, 48);
  EXPECT_EQ(f.ctrl.getNodeMidiNote(0), 48);
  EXPECT_EQ(f.ctrl.getMixerSnapshot().note(0), 48);
}

TEST(Mixer, MuteDisablesSnapshotEmit) {
  Fixture f;
  f.ctrl.setNodeMuted(0, true);
  EXPECT_TRUE(f.ctrl.getNodeMuted(0));
  EXPECT_FALSE(f.ctrl.getMixerSnapshot().enabled(0));

  f.ctrl.setNodeMuted(0, false);
  EXPECT_TRUE(f.ctrl.getMixerSnapshot().enabled(0));
}

TEST(Mixer, SoloDisablesNonSoloedNodes) {
  Fixture f;
  f.ctrl.createChild(0);  // node 1
  f.ctrl.setNodeSoloed(1, true);

  EXPECT_FALSE(f.ctrl.getMixerSnapshot().enabled(0));
  EXPECT_TRUE(f.ctrl.getMixerSnapshot().enabled(1));

  f.ctrl.setNodeSoloed(1, false);
  EXPECT_TRUE(f.ctrl.getMixerSnapshot().enabled(0));
  EXPECT_TRUE(f.ctrl.getMixerSnapshot().enabled(1));
}

TEST(Mixer, MuteBeatsSolo) {
  Fixture f;
  f.ctrl.createChild(0);  // node 1
  f.ctrl.setNodeSoloed(1, true);
  f.ctrl.setNodeMuted(1, true);

  EXPECT_FALSE(f.ctrl.getMixerSnapshot().enabled(1));
}

TEST(Mixer, RebuildRestoresMixerState) {
  Fixture a;
  a.ctrl.createChild(0);  // node 1
  a.ctrl.setNodeVolume(1, 0.42f);
  a.ctrl.setNodeMidiNote(1, 55);
  a.ctrl.setNodeMuted(1, true);
  juce::ValueTree tree = a.ctrl.getTree().createCopy();

  Fixture b;
  b.ctrl.rebuild(tree);

  EXPECT_FLOAT_EQ(b.ctrl.getNodeVolume(1), 0.42f);
  EXPECT_EQ(b.ctrl.getNodeMidiNote(1), 55);
  EXPECT_TRUE(b.ctrl.getNodeMuted(1));
  EXPECT_NEAR(b.ctrl.getMixerSnapshot().gain(1), 0.42f, 1e-6);
  EXPECT_FALSE(b.ctrl.getMixerSnapshot().enabled(1));
}

TEST(Mixer, DeleteNodeDisablesSnapshotSlot) {
  Fixture f;
  f.ctrl.createChild(0);  // node 1
  f.ctrl.deleteNode(1);
  EXPECT_FALSE(f.ctrl.getMixerSnapshot().enabled(1));
}

// ---------------------------------------------------------------------------
// Full processor state round-trip
// ---------------------------------------------------------------------------

TEST(Mixer, StateRoundTripPreservesMixerState) {
  neurythmic::PluginProcessor a;
  a.getController().createChild(0);  // node 1
  a.getController().setNodeVolume(1, 0.25f);
  a.getController().setNodeMidiNote(1, 40);
  a.getController().setNodeMuted(1, true);

  juce::MemoryBlock mb;
  a.getStateInformation(mb);

  neurythmic::PluginProcessor b;
  b.setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));

  EXPECT_FLOAT_EQ(b.getController().getNodeVolume(1), 0.25f);
  EXPECT_EQ(b.getController().getNodeMidiNote(1), 40);
  EXPECT_TRUE(b.getController().getNodeMuted(1));
}

// ---------------------------------------------------------------------------
// MenuValues: note <-> octave/semitone helpers
// ---------------------------------------------------------------------------

TEST(Mixer, NoteOctaveSemitoneHelpers) {
  EXPECT_EQ(neurythmic::MenuValues::midiNoteFromOctaveSemitone(4, 0), 60);
  EXPECT_EQ(neurythmic::MenuValues::midiNoteFromOctaveSemitone(4, 9), 69);
  EXPECT_EQ(neurythmic::MenuValues::midiNoteFromOctaveSemitone(-1, 0), 0);
  EXPECT_EQ(neurythmic::MenuValues::midiNoteFromOctaveSemitone(5, 0), 72);
  EXPECT_EQ(neurythmic::MenuValues::midiNoteFromOctaveSemitone(5, 11), 83);
  EXPECT_EQ(neurythmic::MenuValues::midiNoteFromOctaveSemitone(20, 0), 127);

  EXPECT_EQ(neurythmic::MenuValues::noteOctave(60), 4);
  EXPECT_EQ(neurythmic::MenuValues::noteSemitone(60), 0);
  EXPECT_EQ(neurythmic::MenuValues::noteOctave(69), 4);
  EXPECT_EQ(neurythmic::MenuValues::noteSemitone(69), 9);
}
