#include <gtest/gtest.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Neurythmic/Neurythmic.h"

TEST(NeurythmicPlugin, PluginCanBeCreated)
{
    neurythmic::PluginProcessor processor;
    EXPECT_FALSE(processor.getName().isEmpty());
}

TEST(NeurythmicPlugin, IsInstrumentType)
{
    neurythmic::PluginProcessor processor;
    EXPECT_FALSE(processor.isMidiEffect());
    EXPECT_EQ(processor.getTotalNumInputChannels(), 0);
}

TEST(NeurythmicPlugin, HasEditor)
{
    neurythmic::PluginProcessor processor;
    EXPECT_TRUE(processor.hasEditor());
}

TEST(NeurythmicPlugin, AcceptsMidi)
{
    neurythmic::PluginProcessor processor;
    EXPECT_TRUE(processor.acceptsMidi());
}

TEST(NeurythmicPlugin, ProducesMidi)
{
    neurythmic::PluginProcessor processor;
    EXPECT_TRUE(processor.producesMidi());
}

TEST(NeurythmicPlugin, RejectsInputBuses)
{
    neurythmic::PluginProcessor processor;

    juce::AudioProcessor::BusesLayout inputLayout;
    inputLayout.inputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor.isBusesLayoutSupported(inputLayout));
}

TEST(NeurythmicPlugin, SupportsStereoLayout)
{
    neurythmic::PluginProcessor processor;

    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_TRUE(processor.isBusesLayoutSupported(stereoLayout));
}

TEST(NeurythmicPlugin, SupportsMonoLayout)
{
    neurythmic::PluginProcessor processor;

    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.outputBuses.add(juce::AudioChannelSet::mono());

    EXPECT_TRUE(processor.isBusesLayoutSupported(monoLayout));
}

TEST(NeurythmicPlugin, ProcessBlockDoesNotCrash)
{
    neurythmic::PluginProcessor processor;

    processor.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    EXPECT_NO_THROW(processor.processBlock(buffer, midi));

    processor.releaseResources();
}

TEST(NeurythmicPlugin, GetNumPrograms)
{
    neurythmic::PluginProcessor processor;
    EXPECT_GE(processor.getNumPrograms(), 1);
}

TEST(NeurythmicPlugin, TailLengthIsNonNegative)
{
    neurythmic::PluginProcessor processor;
    EXPECT_GE(processor.getTailLengthSeconds(), 0.0);
}

// CONFIG MANAGER TESTS
TEST(ConfigManager, ParsesCoreSettings) {
    auto& cfg = neurythmic::ConfigManager::get();
    EXPECT_FLOAT_EQ(cfg.nodeRadius, 34.0f);
    EXPECT_FLOAT_EQ(cfg.node0HaloSize, 1.35f);
    EXPECT_FLOAT_EQ(cfg.t1Overt2, 4.0f);
    EXPECT_FLOAT_EQ(cfg.c, 1.0f);
    EXPECT_FLOAT_EQ(cfg.b, 7.0f);
}

TEST(ConfigManager, ParsesCpgBehaviour) {
    auto& cfg = neurythmic::ConfigManager::get();
    EXPECT_FLOAT_EQ(cfg.g, 7.0f);
    EXPECT_FLOAT_EQ(cfg.freqCompensation, 0.9732f);
    EXPECT_FLOAT_EQ(cfg.nodeIntensityFloor, 0.7f);
    EXPECT_FLOAT_EQ(cfg.newNodeFreqMultiple, 2.0f);
    EXPECT_FLOAT_EQ(cfg.newNodeVolumeInit, 0.6f);
    EXPECT_FLOAT_EQ(cfg.newConnWeightScale, 0.0f);
    EXPECT_FLOAT_EQ(cfg.newParentChildConnWeightScale, 1.0f);
    EXPECT_FLOAT_EQ(cfg.nodeDistWeightScalingLimit, 0.85f);
    EXPECT_FLOAT_EQ(cfg.nodeDistWeightScalingStart, 0.01f);
    EXPECT_FLOAT_EQ(cfg.nodeDistWeightScalingExp, 2.0f);
    EXPECT_FLOAT_EQ(cfg.connectionWeightMax, 10.0f);
    EXPECT_FLOAT_EQ(cfg.connectionWeightScalingUnity, 3.0f);
    EXPECT_TRUE(cfg.connectionWeightScalingOn);
    EXPECT_FALSE(cfg.positionMapsToPan);
}

TEST(ConfigManager, ParsesLayoutInteraction) {
    auto& cfg = neurythmic::ConfigManager::get();
    EXPECT_FLOAT_EQ(cfg.nodeClickableRadius, 0.035f);
    EXPECT_FLOAT_EQ(cfg.nodeCollideDistance, 0.07f);
    EXPECT_FLOAT_EQ(cfg.nodeSpawnDistance, 0.12f);
    EXPECT_FLOAT_EQ(cfg.connectionClickableWidth, 15.0f);
}

TEST(ConfigManager, ParsesLayoutRendering) {
    auto& cfg = neurythmic::ConfigManager::get();
    EXPECT_EQ(cfg.pointsInCircle, 30);
    EXPECT_EQ(cfg.pointsInArc, 30);
    EXPECT_FLOAT_EQ(cfg.arrowHeadSize, 12.0f);
    EXPECT_FLOAT_EQ(cfg.arrowHeadWidthMultiplier, 2.0f);
    EXPECT_FLOAT_EQ(cfg.arrowHeadWidthMin, 5.0f);
    EXPECT_FLOAT_EQ(cfg.arrowHeadWidthMax, 13.0f);
    EXPECT_FLOAT_EQ(cfg.arcAdjust, 1.0f);
    EXPECT_FLOAT_EQ(cfg.minArcAdjustRad, 0.0f);
    EXPECT_FLOAT_EQ(cfg.maxArcAdjustRad, 50.0f);
    EXPECT_FLOAT_EQ(cfg.defaultCurveAmount, 0.01f);
    EXPECT_FLOAT_EQ(cfg.minLineThickness, 1.0f);
    EXPECT_FLOAT_EQ(cfg.maxLineThickness, 8.0f);
    EXPECT_FLOAT_EQ(cfg.nodeLineThickness, 2.4f);
    EXPECT_FLOAT_EQ(cfg.nodeFlashSpread, 40.0f);
    EXPECT_FLOAT_EQ(cfg.nodeFlashExponent, 8.0f);
    EXPECT_FLOAT_EQ(cfg.minLineWeight, 0.1f);
    EXPECT_FLOAT_EQ(cfg.maxLineWeight, 7.0f);
    EXPECT_FLOAT_EQ(cfg.connBrightnessParentChild, 1.0f);
    EXPECT_FLOAT_EQ(cfg.connBrightnessOther, 1.0f);
    EXPECT_FLOAT_EQ(cfg.inactiveBrightnessMult, 0.4f);
    EXPECT_FLOAT_EQ(cfg.connectionColourStartsAtLevel, 0.9f);
}

TEST(ConfigManager, ParsesColours) {
    auto& cfg = neurythmic::ConfigManager::get();
    EXPECT_EQ(cfg.connColour.getRed(), static_cast<juce::uint8>(255));
    EXPECT_EQ(cfg.connColour.getGreen(), static_cast<juce::uint8>(150));
    EXPECT_EQ(cfg.connColour.getBlue(), static_cast<juce::uint8>(20));
    EXPECT_EQ(cfg.selectedColour.getGreen(), static_cast<juce::uint8>(255));
    EXPECT_EQ(cfg.fontNodeInfoColour.getRed(), static_cast<juce::uint8>(170));
}

TEST(ConfigManager, ParsesNodeCollections) {
    auto& cfg = neurythmic::ConfigManager::get();

    EXPECT_EQ(cfg.getNodeFreqCount(), 10);
    EXPECT_FLOAT_EQ(cfg.getNodeFreq(0), 0.25f);
    EXPECT_FLOAT_EQ(cfg.getNodeFreq(4), 2.0f);
    EXPECT_FLOAT_EQ(cfg.getNodeFreq(9), 16.0f);

    EXPECT_EQ(cfg.getNodeFreqName(0), juce::String("1/4"));
    EXPECT_EQ(cfg.getNodeFreqName(9), juce::String("16"));

    const auto& freqs = cfg.getNodeFreqs();
    EXPECT_EQ(static_cast<int>(freqs.size()), 10);

    EXPECT_FLOAT_EQ(cfg.getNodePitch(0, 0), 220.0f);
    EXPECT_FLOAT_EQ(cfg.getNodePitch(0, 2), 440.0f);
}

TEST(ConfigManager, ParsesFlags) {
    auto& cfg = neurythmic::ConfigManager::get();
    EXPECT_TRUE(cfg.showReloadSettings);
    EXPECT_TRUE(cfg.showNodeMenu);
    EXPECT_FLOAT_EQ(cfg.mainMenuOpacity, 0.2f);
    EXPECT_EQ(cfg.defaultPresetFile, juce::String("./matsuPresets.pre"));
}

TEST(ConfigManager, ParsesScalingCurve) {
    auto& cfg = neurythmic::ConfigManager::get();
    const auto& x = cfg.getWeightScalingCurveX();
    const auto& y = cfg.getWeightScalingCurveY();
    ASSERT_EQ(x.size(), y.size());
    EXPECT_EQ(static_cast<int>(x.size()), 10);
    EXPECT_FLOAT_EQ(x[0], 0.25f);
    EXPECT_FLOAT_EQ(y[0], 1.955492228f);
    EXPECT_FLOAT_EQ(x[9], 8.0f);
    EXPECT_FLOAT_EQ(y[9], 1.259015544f);
}

// NETWORK STATE TESTS

TEST(NetworkState, InitCreatesSingleRoot) {
    auto tree = neurythmic::NetworkState::initEmptyNetwork();
    EXPECT_TRUE(tree.hasType(neurythmic::NetworkState::IDs::NETWORK));
    EXPECT_EQ(tree.getNumChildren(), 1);

    auto root = neurythmic::NetworkState::getNode(tree, 0);
    EXPECT_TRUE(root.isValid());
    EXPECT_EQ(static_cast<int>(root.getProperty(neurythmic::NetworkState::Props::id)), 0);
    EXPECT_FLOAT_EQ(static_cast<float>(root.getProperty(neurythmic::NetworkState::Props::positionX)), 0.5f);
}

TEST(NetworkState, CreateAndLookupNodes) {
    auto tree = neurythmic::NetworkState::initEmptyNetwork();
    neurythmic::NetworkState::createNode(tree, 1, 0, 0.25f, 0.75f);
    neurythmic::NetworkState::createNode(tree, 2, 0, 0.75f, 0.25f);

    EXPECT_EQ(tree.getNumChildren(), 3);

    auto n1 = neurythmic::NetworkState::getNode(tree, 1);
    EXPECT_TRUE(n1.isValid());
    EXPECT_EQ(static_cast<int>(n1.getProperty(neurythmic::NetworkState::Props::parentId)), 0);

    auto pc = n1.getChildWithProperty(neurythmic::NetworkState::Props::sourceId, 0);
    EXPECT_TRUE(pc.isValid());
    EXPECT_TRUE(pc.hasType(neurythmic::NetworkState::IDs::CONNECTION));
}

TEST(NetworkState, RemoveNode) {
    auto tree = neurythmic::NetworkState::initEmptyNetwork();
    neurythmic::NetworkState::createNode(tree, 1, 0, 0.25f, 0.75f);
    neurythmic::NetworkState::removeNode(tree, 1);

    EXPECT_EQ(tree.getNumChildren(), 1);
    EXPECT_FALSE(neurythmic::NetworkState::getNode(tree, 1).isValid());
}

TEST(NetworkState, ConnectionsAndIteration) {
    auto tree = neurythmic::NetworkState::initEmptyNetwork();
    neurythmic::NetworkState::createNode(tree, 1, 0, 0.25f, 0.75f);
    neurythmic::NetworkState::createNode(tree, 2, 0, 0.75f, 0.25f);

    auto n1 = neurythmic::NetworkState::getNode(tree, 1);
    neurythmic::NetworkState::createConnection(n1, 2, 0.5, 0.25);

    int nodeCount = 0;
    neurythmic::NetworkState::forEachNode(tree,
        [&](juce::ValueTree) { ++nodeCount; });
    EXPECT_EQ(nodeCount, 3);

    int connCount = 0;
    neurythmic::NetworkState::forEachConnection(n1,
        [&](juce::ValueTree) { ++connCount; });
    EXPECT_EQ(connCount, 2);  // parent-child edge + the extra input

    neurythmic::NetworkState::removeConnection(n1, 2);
    EXPECT_FALSE(n1.getChildWithProperty(neurythmic::NetworkState::Props::sourceId, 2).isValid());
}
