#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Neurythmic/Neurythmic.h"

namespace {

void setParam(neurythmic::PluginProcessor& processor,
              const char* id,
              float value) {
  auto& apvts = processor.getAPVTS();
  auto* param = apvts.getParameter(id);
  ASSERT_NE(param, nullptr) << id;
  auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
  ASSERT_NE(ranged, nullptr) << id;
  param->setValueNotifyingHost(ranged->convertTo0to1(value));
}

double treeNodeFreq(const neurythmic::NetworkController& controller,
                    int nodeId) {
  return static_cast<double>(
      neurythmic::NetworkState::getNode(controller.getTree(), nodeId)
          .getProperty(neurythmic::NetworkState::Props::freq));
}

}  // namespace

// ---------------------------------------------------------------------------
// S1: updateFromEngine must not write engine Hz into canonical properties
// ---------------------------------------------------------------------------

TEST(UpdateFromEngine, PreservesChildMultiples) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int child = controller.createChild(0);
  controller.setNodeFrequencyMultiple(child, 2.0);

  for (int i = 0; i < 2000; ++i)
    engine.step();

  controller.updateFromEngine();

  EXPECT_DOUBLE_EQ(controller.getNodeFrequencyMultiple(child), 2.0);
  EXPECT_DOUBLE_EQ(treeNodeFreq(controller, child), 2.0);
  EXPECT_NEAR(controller.getNodeFrequency(child),
              engine.getNodeFrequency(0) * 2.0, 1e-6);
}

TEST(UpdateFromEngine, PreservesCanonicalRoot) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());

  auto root = neurythmic::NetworkState::getNode(
      controller.getTree(), neurythmic::NetworkState::kRootNodeId);
  root.setProperty(neurythmic::NetworkState::Props::freq, 2.5, nullptr);

  // Simulate the host-driven bar root.
  engine.setNodeFrequency(0, 0.5, true);
  engine.doQueuedActions();
  for (int i = 0; i < 100; ++i)
    engine.step();

  controller.updateFromEngine();

  EXPECT_DOUBLE_EQ(treeNodeFreq(controller, 0), 2.5);
}

// ---------------------------------------------------------------------------
// S2: tree and engine quantiser state must agree
// ---------------------------------------------------------------------------

TEST(QuantiserState, NewChildMatchesTree) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int child = controller.createChild(0);

  EXPECT_EQ(static_cast<int>(controller.getNodeQuantiseGrid(child)),
            static_cast<int>(engine.getNodeQuantiser_Grid(child)));
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseMultiple(child),
                  engine.getNodeQuantiser_Multiple(child));
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseOffset(child),
                  engine.getNodeQuantiser_Offset(child));
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseAmount(child),
                  engine.getQuantiseAmount(child));

  EXPECT_FLOAT_EQ(controller.getNodeQuantiseMultiple(child), 8.0f);
  EXPECT_EQ(static_cast<int>(controller.getNodeQuantiseGrid(child)),
            static_cast<int>(MatsuokaEngine::gridType::unQuantised));
}

TEST(QuantiserState, RecreatedNodeUsesDefaults) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int child = controller.createChild(0);

  controller.setNodeQuantiseGrid(child, MatsuokaEngine::gridType::_32nd);
  controller.setNodeQuantiseMultiple(child, 4.0f);
  controller.setNodeQuantiseOffset(child, 3.0f);
  controller.setNodeQuantiseAmount(child, 0.2f);
  controller.deleteNode(child);

  const int recreated = controller.createChild(0);
  EXPECT_EQ(recreated, child);

  EXPECT_EQ(static_cast<int>(controller.getNodeQuantiseGrid(recreated)),
            static_cast<int>(engine.getNodeQuantiser_Grid(recreated)));
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseMultiple(recreated),
                  engine.getNodeQuantiser_Multiple(recreated));
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseOffset(recreated),
                  engine.getNodeQuantiser_Offset(recreated));
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseAmount(recreated),
                  engine.getQuantiseAmount(recreated));

  EXPECT_FLOAT_EQ(controller.getNodeQuantiseMultiple(recreated), 8.0f);
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseOffset(recreated), 0.0f);
  EXPECT_FLOAT_EQ(controller.getNodeQuantiseAmount(recreated), 1.0f);
}

// ---------------------------------------------------------------------------
// S3: preset state must include APVTS master/root parameters
// ---------------------------------------------------------------------------

TEST(PresetState, RoundTripsRootAndMasterParameters) {
  neurythmic::PluginProcessor source;
  setParam(source, "rootFreq", 3.0f);
  setParam(source, "masterVolume", 0.33f);
  setParam(source, "masterQuantAmount", 0.22f);
  setParam(source, "velocityMode", 0.0f);
  setParam(source, "routingMode", 1.0f);

  const int child = source.getController().createChild(0);
  source.getController().setNodeFrequencyMultiple(child, 3.0);

  juce::MemoryBlock state;
  source.getStateInformation(state);
  ASSERT_GT(state.getSize(), 0u);

  neurythmic::PluginProcessor loaded;
  loaded.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

  EXPECT_NEAR(loaded.getAPVTS().getRawParameterValue("rootFreq")->load(), 3.0f,
              1e-4f);
  EXPECT_NEAR(
      loaded.getAPVTS().getRawParameterValue("masterVolume")->load(), 0.33f,
      1e-4f);
  EXPECT_NEAR(
      loaded.getAPVTS().getRawParameterValue("masterQuantAmount")->load(), 0.22f,
      1e-4f);
  EXPECT_FALSE(loaded.getAPVTS().getRawParameterValue("velocityMode")->load() >
               0.5f);
  EXPECT_GT(loaded.getAPVTS().getRawParameterValue("routingMode")->load(), 0.5f);
  EXPECT_DOUBLE_EQ(loaded.getController().getNodeFrequencyMultiple(child), 3.0);
  EXPECT_NEAR(treeNodeFreq(loaded.getController(), 0), 3.0, 1e-4);
}

// ---------------------------------------------------------------------------
// S3/S6: trigger-mode switch and preset round-trip
// ---------------------------------------------------------------------------

TEST(TriggerMode, ApvtsSwitchReachesEngine) {
  neurythmic::PluginProcessor processor;
  auto* param = processor.getAPVTS().getParameter("fireOnPeak");
  ASSERT_NE(param, nullptr);

  setParam(processor, "fireOnPeak", 0.0f);
  EXPECT_FALSE(processor.getEngine().getFireOnPeak());

  setParam(processor, "fireOnPeak", 1.0f);
  EXPECT_TRUE(processor.getEngine().getFireOnPeak());
}

TEST(TriggerMode, PresetRoundTripsSwitch) {
  neurythmic::PluginProcessor source;
  setParam(source, "fireOnPeak", 0.0f);

  juce::MemoryBlock state;
  source.getStateInformation(state);

  neurythmic::PluginProcessor loaded;
  loaded.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

  EXPECT_FALSE(loaded.getAPVTS().getRawParameterValue("fireOnPeak")->load() >
               0.5f);
  EXPECT_FALSE(loaded.getEngine().getFireOnPeak());
}

// ---------------------------------------------------------------------------
// S5: message-thread servicing drains visual events without an editor
// ---------------------------------------------------------------------------

TEST(VisualEvents, MessageThreadDrainsWithoutEditor) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource tempo;  // hasHost = false, isPlaying = false
  processor.setTempoSource(&tempo);
  processor.setRateAndBufferSizeDetails(44100.0, 512);
  processor.prepareToPlay(44100.0, 512);

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  for (int block = 0; block < 200; ++block)
    processor.processBlock(buffer, midi);

  EXPECT_GT(processor.getEngine().getPendingEventCount(), 0u);
  processor.serviceMessageThread();
  EXPECT_EQ(processor.getEngine().getPendingEventCount(), 0u);
}

TEST(PresetState, FilePresetRoundTripsMasterAndRoot) {
  neurythmic::PluginProcessor source;
  setParam(source, "rootFreq", 4.0f);
  setParam(source, "masterVolume", 0.21f);
  setParam(source, "fireOnPeak", 0.0f);

  const int child = source.getController().createChild(0);
  source.getController().setNodeFrequencyMultiple(child, 2.5);

  neurythmic::PresetManager sourcePresets(source.getController(), &source);
  auto tmp = juce::File::createTempFile(".nprs");
  ASSERT_TRUE(sourcePresets.savePreset(tmp));

  neurythmic::PluginProcessor loaded;
  neurythmic::PresetManager loadedPresets(loaded.getController(), &loaded);
  ASSERT_TRUE(loadedPresets.loadPreset(tmp));

  EXPECT_NEAR(loaded.getAPVTS().getRawParameterValue("rootFreq")->load(), 4.0f,
              1e-4f);
  EXPECT_NEAR(
      loaded.getAPVTS().getRawParameterValue("masterVolume")->load(), 0.21f,
      1e-4f);
  EXPECT_FALSE(loaded.getEngine().getFireOnPeak());
  EXPECT_DOUBLE_EQ(loaded.getController().getNodeFrequencyMultiple(child), 2.5);

  tmp.deleteFile();
}

TEST(PresetState, LoadWhilePlayingReappliesHostRoot) {
  neurythmic::PluginProcessor processor;
  neurythmic::FakeTempoSource tempo;
  tempo.info.hasHost = true;
  tempo.info.isPlaying = true;
  tempo.info.bpm = 120.0;
  tempo.info.hasPpq = true;
  tempo.info.ppqPosition = 0.0;
  tempo.info.timeSigNumerator = 4;
  tempo.info.timeSigDenominator = 4;
  processor.setTempoSource(&tempo);
  processor.setRateAndBufferSizeDetails(44100.0, 512);
  processor.prepareToPlay(44100.0, 512);

  juce::AudioBuffer<float> buffer(2, 512);
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);
  ASSERT_NEAR(processor.getEngine().getNodeFrequency(0), 0.5, 1e-4);

  juce::MemoryBlock state;
  processor.getStateInformation(state);
  processor.setStateInformation(state.getData(),
                                static_cast<int>(state.getSize()));

  // rebuild() writes the loaded standalone root (2.0 Hz) into the engine. The
  // next block must restore the host bar root (0.5 Hz at 120 BPM 4/4).
  processor.processBlock(buffer, midi);
  EXPECT_NEAR(processor.getEngine().getNodeFrequency(0), 0.5, 1e-4);
}
