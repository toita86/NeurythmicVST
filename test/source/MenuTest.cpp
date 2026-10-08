#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

#include <cmath>
#include <vector>

#include "MatsuokaEngine.h"
#include "Neurythmic/ConfigManager.h"
#include "Neurythmic/MenuValues.h"
#include "Neurythmic/NetworkController.h"
#include "Neurythmic/NetworkState.h"

namespace {

using neurythmic::NetworkController;
using neurythmic::ConfigManager;
namespace MenuValues = neurythmic::MenuValues;
namespace NetworkState = neurythmic::NetworkState;

}  // namespace

// Node parameter mutators (Node tab) -----------------------------------------

TEST(Menu, SetNodeFrequencyRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // node 1

  ctrl.setNodeFrequency(1, 3.5, false);
  EXPECT_DOUBLE_EQ(engine.getNodeFrequency(1), 3.5);
  EXPECT_DOUBLE_EQ(ctrl.getNodeFrequency(1), 3.5);
  EXPECT_DOUBLE_EQ(
      static_cast<double>(
          NetworkState::getNode(ctrl.getTree(), 1)
              .getProperty(NetworkState::Props::freq)),
      3.5);
}

TEST(Menu, SetNodeSelfNoiseRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  ctrl.setNodeSelfNoise(1, 0.3);
  EXPECT_DOUBLE_EQ(engine.getNodeSelfNoise(1), 0.3);
  EXPECT_DOUBLE_EQ(ctrl.getNodeSelfNoise(1), 0.3);
  EXPECT_DOUBLE_EQ(
      static_cast<double>(
          NetworkState::getNode(ctrl.getTree(), 1)
              .getProperty(NetworkState::Props::noise)),
      0.3);
}

TEST(Menu, SetNodePhaseOffsetRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  ctrl.setNodePhaseOffset(1, 0.25);
  // The engine stores phase as a sample-quantised delay, so round-trips
  // approximately; the ValueTree (source of truth) is exact.
  EXPECT_NEAR(engine.getNodePhaseOffset(1), 0.25, 1e-3);
  EXPECT_DOUBLE_EQ(ctrl.getNodePhaseOffset(1), 0.25);
  EXPECT_DOUBLE_EQ(
      static_cast<double>(
          NetworkState::getNode(ctrl.getTree(), 1)
              .getProperty(NetworkState::Props::phaseOffset)),
      0.25);
}

TEST(Menu, SetNodeSynchModeRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  ctrl.setNodeSynchMode(1, MatsuNode::synchMode::synchOnce);
  EXPECT_EQ(engine.getNodeSynchMode(1), MatsuNode::synchMode::synchOnce);
  EXPECT_EQ(ctrl.getNodeSynchMode(1), MatsuNode::synchMode::synchOnce);
  EXPECT_EQ(
      static_cast<int>(
          NetworkState::getNode(ctrl.getTree(), 1)
              .getProperty(NetworkState::Props::synchMode)),
      1);

  ctrl.setNodeSynchMode(1, MatsuNode::synchMode::synchLock);
  EXPECT_EQ(ctrl.getNodeSynchMode(1), MatsuNode::synchMode::synchLock);
  EXPECT_EQ(
      static_cast<int>(
          NetworkState::getNode(ctrl.getTree(), 1)
              .getProperty(NetworkState::Props::synchMode)),
      2);
}

// Quantise mutators (Constraint tab) -----------------------------------------

TEST(Menu, SetNodeQuantiseGridRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  ctrl.setNodeQuantiseGrid(1, MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(engine.getNodeQuantiser_Grid(1), MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(ctrl.getNodeQuantiseGrid(1), MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(
      static_cast<int>(
          NetworkState::getNode(ctrl.getTree(), 1)
              .getProperty(NetworkState::Props::quantGrid)),
      1);
}

TEST(Menu, SetNodeQuantiseMultipleRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  ctrl.setNodeQuantiseMultiple(1, 4.0f);
  EXPECT_FLOAT_EQ(engine.getNodeQuantiser_Multiple(1), 4.0f);
  EXPECT_FLOAT_EQ(ctrl.getNodeQuantiseMultiple(1), 4.0f);
}

TEST(Menu, SetNodeQuantiseOffsetRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  // The quantiser stores the offset as an unsigned grid position.
  ctrl.setNodeQuantiseOffset(1, 2.0f);
  EXPECT_FLOAT_EQ(engine.getNodeQuantiser_Offset(1), 2.0f);
  EXPECT_FLOAT_EQ(ctrl.getNodeQuantiseOffset(1), 2.0f);
}

TEST(Menu, SetNodeQuantiseAmountRoundTrips) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  ctrl.setNodeQuantiseAmount(1, 0.2f);
  EXPECT_FLOAT_EQ(engine.getQuantiseAmount(1), 0.2f);
  EXPECT_FLOAT_EQ(ctrl.getNodeQuantiseAmount(1), 0.2f);
}

// Connection menu getters ----------------------------------------------------

TEST(Menu, ConnectionGettersRoundTrip) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1
  ctrl.createChild(0);  // 2
  ctrl.addConnection(1, 2);

  ctrl.setConnectionScaleFactor(1, 2, 2.5);
  EXPECT_DOUBLE_EQ(ctrl.getConnectionScaleFactor(1, 2), 2.5);
  EXPECT_DOUBLE_EQ(ctrl.getConnectionWeight(1, 2),
                   ctrl.calcWeight(1, 2, 2.5));

  ctrl.updateConnectionPhase(1, 2, 0.3);
  EXPECT_DOUBLE_EQ(ctrl.getConnectionPhase(1, 2), 0.3);
}

TEST(Menu, ParentEdgeWeightGetter) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1, parent edge 0->1

  EXPECT_DOUBLE_EQ(ctrl.getConnectionWeight(0, 1),
                   ctrl.calcWeight(0, 1, 1.0));
}

// Parameter persistence across a preset rebuild ------------------------------

TEST(Menu, RebuildRestoresNodeParameters) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1

  ctrl.setNodeSelfNoise(1, 0.3);
  ctrl.setNodePhaseOffset(1, 0.25);
  ctrl.setNodeSynchMode(1, MatsuNode::synchMode::synchLock);
  ctrl.setNodeQuantiseGrid(1, MatsuokaEngine::gridType::_32nd);
  ctrl.setNodeQuantiseMultiple(1, 2.0f);
  ctrl.setNodeQuantiseOffset(1, 2.0f);
  ctrl.setNodeQuantiseAmount(1, 0.4f);

  juce::ValueTree saved = ctrl.getTree().createCopy();
  ctrl.clear();
  ctrl.rebuild(saved);

  EXPECT_DOUBLE_EQ(ctrl.getNodeSelfNoise(1), 0.3);
  EXPECT_DOUBLE_EQ(ctrl.getNodePhaseOffset(1), 0.25);
  EXPECT_EQ(ctrl.getNodeSynchMode(1), MatsuNode::synchMode::synchLock);
  EXPECT_EQ(ctrl.getNodeQuantiseGrid(1), MatsuokaEngine::gridType::_32nd);
  EXPECT_FLOAT_EQ(ctrl.getNodeQuantiseMultiple(1), 2.0f);
  EXPECT_FLOAT_EQ(ctrl.getNodeQuantiseOffset(1), 2.0f);
  EXPECT_FLOAT_EQ(ctrl.getNodeQuantiseAmount(1), 0.4f);
}

// MenuValues pure mapping ----------------------------------------------------

TEST(MenuValues, FrequencyMultiplesAndLabels) {
  EXPECT_EQ(MenuValues::frequencyCount(), 10);
  EXPECT_DOUBLE_EQ(MenuValues::frequencyMultiple(0), 0.25);
  EXPECT_DOUBLE_EQ(MenuValues::frequencyMultiple(3), 1.0);
  EXPECT_DOUBLE_EQ(MenuValues::frequencyMultiple(4), 2.0);
  EXPECT_DOUBLE_EQ(MenuValues::frequencyMultiple(9), 16.0);
  EXPECT_EQ(MenuValues::frequencyLabel(0), juce::String("1/4"));
  EXPECT_EQ(MenuValues::frequencyLabel(9), juce::String("16"));
}

TEST(MenuValues, NearestFrequencyIndexSnaps) {
  EXPECT_EQ(MenuValues::nearestFrequencyIndex(1.0), 3);
  EXPECT_EQ(MenuValues::nearestFrequencyIndex(2.0), 4);
  EXPECT_EQ(MenuValues::nearestFrequencyIndex(16.0), 9);
  EXPECT_EQ(MenuValues::nearestFrequencyIndex(0.5), 2);
  EXPECT_EQ(MenuValues::nearestFrequencyIndex(0.25), 0);
}

TEST(MenuValues, SynchModeMapping) {
  EXPECT_EQ(MenuValues::synchModeFromIndex(0), MatsuNode::synchMode::free);
  EXPECT_EQ(MenuValues::synchModeFromIndex(1), MatsuNode::synchMode::synchOnce);
  EXPECT_EQ(MenuValues::synchModeFromIndex(2), MatsuNode::synchMode::synchLock);
  EXPECT_EQ(MenuValues::synchModeToIndex(MatsuNode::synchMode::free), 0);
  EXPECT_EQ(MenuValues::synchModeToIndex(MatsuNode::synchMode::synchOnce), 1);
  EXPECT_EQ(MenuValues::synchModeToIndex(MatsuNode::synchMode::synchLock), 2);
}

TEST(MenuValues, GridTypeMapping) {
  EXPECT_EQ(MenuValues::gridFromIndex(0), MatsuokaEngine::gridType::unQuantised);
  EXPECT_EQ(MenuValues::gridFromIndex(1), MatsuokaEngine::gridType::_24th);
  EXPECT_EQ(MenuValues::gridFromIndex(2), MatsuokaEngine::gridType::_32nd);
  EXPECT_EQ(MenuValues::gridToIndex(MatsuokaEngine::gridType::unQuantised), 0);
  EXPECT_EQ(MenuValues::gridToIndex(MatsuokaEngine::gridType::_24th), 1);
  EXPECT_EQ(MenuValues::gridToIndex(MatsuokaEngine::gridType::_32nd), 2);
}

TEST(MenuValues, ResolutionMultiples) {
  const auto m = MenuValues::resolutionMultiples();
  ASSERT_EQ(static_cast<int>(m.size()), 4);
  EXPECT_FLOAT_EQ(m[0], 8.0f);
  EXPECT_FLOAT_EQ(m[1], 4.0f);
  EXPECT_FLOAT_EQ(m[2], 2.0f);
  EXPECT_FLOAT_EQ(m[3], 1.0f);
}

TEST(MenuValues, ResolutionLabelsByGrid) {
  const auto l24 = MenuValues::resolutionLabels(MatsuokaEngine::gridType::_24th);
  ASSERT_EQ(static_cast<int>(l24.size()), 4);
  EXPECT_EQ(l24[0], juce::String("3"));
  EXPECT_EQ(l24[1], juce::String("6"));
  EXPECT_EQ(l24[2], juce::String("12"));
  EXPECT_EQ(l24[3], juce::String("24"));

  const auto l32 = MenuValues::resolutionLabels(MatsuokaEngine::gridType::_32nd);
  ASSERT_EQ(static_cast<int>(l32.size()), 4);
  EXPECT_EQ(l32[0], juce::String("4"));
  EXPECT_EQ(l32[1], juce::String("8"));
  EXPECT_EQ(l32[2], juce::String("16"));
  EXPECT_EQ(l32[3], juce::String("32"));

  EXPECT_TRUE(
      MenuValues::resolutionLabels(MatsuokaEngine::gridType::unQuantised)
          .empty());
}
