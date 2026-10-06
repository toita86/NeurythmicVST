#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

#include <cmath>

#include "MatsuokaEngine.h"
#include "Neurythmic/GraphGeometry.h"
#include "Neurythmic/NetworkController.h"
#include "Neurythmic/NetworkState.h"

namespace {

using neurythmic::NetworkController;
using neurythmic::ConfigManager;
namespace NetworkState = neurythmic::NetworkState;

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

// Selection ---------------------------------------------------------------

TEST(Interaction, SelectionTogglesAndClears) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // node 1

  EXPECT_FALSE(ctrl.isNodeSelected(1));
  EXPECT_TRUE(ctrl.toggleNodeSelected(1));
  EXPECT_TRUE(ctrl.isNodeSelected(1));
  EXPECT_FALSE(ctrl.toggleNodeSelected(1));
  EXPECT_FALSE(ctrl.isNodeSelected(1));

  ctrl.setNodeSelected(1, true);
  ctrl.selectConnection(1, 2);
  EXPECT_TRUE(ctrl.isConnectionSelected(1, 2));
  ctrl.clearSelection();
  EXPECT_FALSE(ctrl.isNodeSelected(1));
  EXPECT_FALSE(ctrl.isConnectionSelected(1, 2));
}

// Connection hit testing ---------------------------------------------------

TEST(Interaction, ConnectionAtPointHitsParentEdge) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);                       // node 1 (parent 0)
  ctrl.moveNode(1, {0.5f, 0.9f});            // push it well below the root

  const float minDim = 500.0f;
  // Midpoint of the vertical parent edge 0->1, in pixel space.
  const auto hit = ctrl.connectionAtPoint({250.0f, 350.0f}, minDim);
  EXPECT_EQ(hit.first, 0);
  EXPECT_EQ(hit.second, 1);

  // Far to the side of the line -> nothing.
  const auto miss = ctrl.connectionAtPoint({280.0f, 350.0f}, minDim);
  EXPECT_EQ(miss.first, -1);
}

TEST(Interaction, ConnectionAtPointHitsInputEdge) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1
  ctrl.createChild(0);  // 2
  ctrl.moveNode(1, {0.3f, 0.5f});
  ctrl.moveNode(2, {0.7f, 0.5f});
  ctrl.addConnection(1, 2);

  const float minDim = 500.0f;
  const float scaling = std::min(1.0f, minDim / 1000.0f);
  const auto from = ctrl.getNodePosition(1) * minDim;
  const auto to = ctrl.getNodePosition(2) * minDim;
  const auto geo = neurythmic::GraphGeometry::makeConnectionGeometry(
      from, to, /*isParent=*/false, scaling, ConfigManager::get());

  // A point on the drawn arc: the draw-start direction scaled to the arc radius.
  const float d = geo.start.getDistanceFrom(geo.arcCentre);
  const auto dir = (geo.start - geo.arcCentre) * (geo.radius / d);
  const auto onArc = geo.arcCentre + dir;

  const auto hit = ctrl.connectionAtPoint(onArc, minDim);
  EXPECT_EQ(hit.first, 1);
  EXPECT_EQ(hit.second, 2);
}

TEST(Interaction, ConnectionAtPointMissesEmptySpace) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);

  const auto hit = ctrl.connectionAtPoint({10.0f, 10.0f}, 500.0f);
  EXPECT_EQ(hit.first, -1);
  EXPECT_EQ(hit.second, -1);
}

// Drag ---------------------------------------------------------------------

TEST(Interaction, MoveSelectedNodesUpdatesPositionAndWeight) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // node 1

  const auto start = ctrl.getNodePosition(1);
  ctrl.setNodeSelected(1, true);
  ctrl.setNodePositionOffsets(start);  // clickOffset = 0
  ctrl.moveSelectedNodes({0.5f, 0.9f});

  EXPECT_FLOAT_EQ(ctrl.getNodePosition(1).getX(), 0.5f);
  EXPECT_FLOAT_EQ(ctrl.getNodePosition(1).getY(), 0.9f);

  // The parent edge weight is recomputed from its scale factor.
  auto conn = NetworkState::getNode(ctrl.getTree(), 1)
                  .getChildWithProperty(NetworkState::Props::sourceId, 0);
  EXPECT_NEAR(static_cast<double>(conn.getProperty(NetworkState::Props::weight)),
              ctrl.calcWeight(0, 1, 1.0), 1e-9);
  EXPECT_DOUBLE_EQ(
      static_cast<double>(conn.getProperty(NetworkState::Props::scaleFactor)),
      1.0);
}

TEST(Interaction, MoveAllNodesTranslatesAll) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1
  ctrl.createChild(0);  // 2

  ctrl.moveAllNodes({0.5f, 0.5f});  // establishes offsets
  ctrl.moveAllNodes({0.6f, 0.6f});  // translates everything by +0.1

  EXPECT_FLOAT_EQ(ctrl.getNodePosition(0).getX(), 0.6f);
  EXPECT_FLOAT_EQ(ctrl.getNodePosition(0).getY(), 0.6f);
  EXPECT_TRUE(ctrl.isNodeSelected(0));

  ctrl.endMoveAllNodes();
  EXPECT_FALSE(ctrl.isNodeSelected(0));
}

// Connection toggling -------------------------------------------------------

TEST(Interaction, ToggleConnectionAddsAndRemoves) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1
  ctrl.createChild(0);  // 2

  ctrl.toggleConnection(1, 2);
  EXPECT_TRUE(ctrl.getIsConnected(1, 2));

  ctrl.toggleConnection(1, 2);
  EXPECT_FALSE(ctrl.getIsConnected(1, 2));
}

TEST(Interaction, ToggleConnectionRefusesParentEdge) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1 (parent edge 0->1)

  ctrl.toggleConnection(0, 1);
  // Parent edge cannot be removed via toggle.
  EXPECT_TRUE(ctrl.getIsConnected(0, 1));
}

// Scale factor defaults -----------------------------------------------------

TEST(Interaction, ScaleFactorStoredOnParentAndInputEdges) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);  // 1
  ctrl.createChild(0);  // 2
  ctrl.addConnection(1, 2);

  auto parentConn = NetworkState::getNode(ctrl.getTree(), 1)
                        .getChildWithProperty(NetworkState::Props::sourceId, 0);
  EXPECT_DOUBLE_EQ(
      static_cast<double>(parentConn.getProperty(NetworkState::Props::scaleFactor)),
      static_cast<double>(ConfigManager::get().newParentChildConnWeightScale));

  auto inputConn = NetworkState::getNode(ctrl.getTree(), 2)
                       .getChildWithProperty(NetworkState::Props::sourceId, 1);
  EXPECT_DOUBLE_EQ(
      static_cast<double>(inputConn.getProperty(NetworkState::Props::scaleFactor)),
      static_cast<double>(ConfigManager::get().newConnWeightScale));
}

// Reset ---------------------------------------------------------------------

TEST(Interaction, ResetNodeRuns) {
  MatsuokaEngine engine(44100);
  NetworkController ctrl(engine, ConfigManager::get());
  ctrl.createChild(0);
  EXPECT_NO_THROW(ctrl.resetNode(1));
}
