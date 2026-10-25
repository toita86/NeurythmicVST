#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include "MatsuokaEngine.h"
#include "Neurythmic/Neurythmic.h"

namespace {

bool hasConnection(const juce::ValueTree& root, int from, int to) {
  auto node = neurythmic::NetworkState::getNode(root, to);
  return node.isValid() &&
         node.getChildWithProperty(neurythmic::NetworkState::Props::sourceId,
                                   from)
             .isValid();
}

}  // namespace

TEST(DeleteSelection, DeletesSelectedLeafAndClearsSelection) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int child = controller.createChild(0);

  controller.clearSelection();
  controller.setNodeSelected(child, true);

  EXPECT_EQ(controller.deleteSelection(), 1);
  EXPECT_FALSE(
      neurythmic::NetworkState::getNode(controller.getTree(), child).isValid());
  EXPECT_FALSE(controller.isNodeSelected(child));
}

TEST(DeleteSelection, SkipsRoot) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());

  controller.clearSelection();
  controller.setNodeSelected(neurythmic::NetworkState::kRootNodeId, true);

  EXPECT_EQ(controller.deleteSelection(), 0);
  EXPECT_TRUE(neurythmic::NetworkState::getNode(
                  controller.getTree(), neurythmic::NetworkState::kRootNodeId)
                  .isValid());
}

TEST(DeleteSelection, SkipsInteriorNodes) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int parent = controller.createChild(0);
  const int child = controller.createChild(parent);

  controller.clearSelection();
  controller.setNodeSelected(parent, true);

  EXPECT_EQ(controller.deleteSelection(), 0);
  EXPECT_TRUE(neurythmic::NetworkState::getNode(controller.getTree(), parent)
                  .isValid());
  EXPECT_TRUE(neurythmic::NetworkState::getNode(controller.getTree(), child)
                  .isValid());
}

TEST(DeleteSelection, DeletesMultipleSelectedLeaves) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int first = controller.createChild(0);
  const int second = controller.createChild(0);

  controller.clearSelection();
  controller.setNodeSelected(first, true);
  controller.setNodeSelected(second, true);

  EXPECT_EQ(controller.deleteSelection(), 2);
  EXPECT_FALSE(
      neurythmic::NetworkState::getNode(controller.getTree(), first).isValid());
  EXPECT_FALSE(
      neurythmic::NetworkState::getNode(controller.getTree(), second).isValid());
}

TEST(DeleteSelection, DeletesSelectedConnection) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int from = controller.createChild(0);
  const int to = controller.createChild(0);
  controller.addConnection(from, to);

  controller.clearSelection();
  controller.selectConnection(from, to);

  EXPECT_EQ(controller.deleteSelection(), 1);
  EXPECT_FALSE(controller.isConnectionSelected(from, to));
  EXPECT_FALSE(hasConnection(controller.getTree(), from, to));
}

TEST(DeleteSelection, RefusesParentEdge) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int child = controller.createChild(0);

  controller.clearSelection();
  controller.selectConnection(neurythmic::NetworkState::kRootNodeId, child);

  EXPECT_EQ(controller.deleteSelection(), 0);
  EXPECT_TRUE(hasConnection(controller.getTree(),
                            neurythmic::NetworkState::kRootNodeId, child));
  EXPECT_TRUE(
      neurythmic::NetworkState::getNode(controller.getTree(), child).isValid());
}

TEST(DeleteSelection, DeleteNodeRemovesStaleSelection) {
  MatsuokaEngine engine(44100);
  neurythmic::NetworkController controller(engine,
                                           neurythmic::ConfigManager::get());
  const int child = controller.createChild(0);

  controller.clearSelection();
  controller.setNodeSelected(child, true);
  controller.deleteNode(child);

  EXPECT_FALSE(controller.isNodeSelected(child));
}
