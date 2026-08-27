#include "../include/Neurythmic/NetworkState.h"
#include <functional>
#include "juce_data_structures/juce_data_structures.h"

namespace neurythmic::NetworkState {

juce::ValueTree initEmptyNetwork() {
  juce::ValueTree network(IDs::NETWORK);
  createNode(network, kRootNodeId, kNoParentId, 0.5f, 0.5f);
  return network;
}

juce::ValueTree getNode(const juce::ValueTree& root, int nodeId) {
  return root.getChildWithProperty(Props::id, nodeId);
}

juce::ValueTree createNode(juce::ValueTree root,
                           int nodeId,
                           int parentId,
                           float posX,
                           float posY) {
  juce::ValueTree node(IDs::NODE);
  node.setProperty(Props::id, nodeId, nullptr);
  node.setProperty(Props::parentId, parentId, nullptr);
  node.setProperty(Props::positionX, posX, nullptr);
  node.setProperty(Props::positionY, posY, nullptr);
  node.setProperty(Props::freq, 0.0, nullptr);
  node.setProperty(Props::phaseOffset, 0.0, nullptr);
  node.setProperty(Props::noise, 0.0, nullptr);
  node.setProperty(Props::synchMode, 0, nullptr);
  node.setProperty(Props::quantGrid, 0, nullptr);
  node.setProperty(Props::quantMultiple, 1, nullptr);
  node.setProperty(Props::quantOffset, 0, nullptr);
  node.setProperty(Props::quantAmount, 1.0f, nullptr);

  root.addChild(node, -1, nullptr);

  if (parentId != kNoParentId)
    createConnection(node, parentId, 1.0, 0.0);  // placeholder weight (1.3)

  return node;
}

void removeNode(juce::ValueTree root, int nodeId) {
  juce::ValueTree node = getNode(root, nodeId);
  if (node.isValid())
    root.removeChild(node, nullptr);
}

juce::ValueTree createConnection(juce::ValueTree target,
                                 int sourceId,
                                 double weight,
                                 double phase) {
  juce::ValueTree existing =
      target.getChildWithProperty(Props::sourceId, sourceId);
  if (existing.isValid())
    return existing;

  juce::ValueTree conn(IDs::CONNECTION);
  conn.setProperty(Props::sourceId, sourceId, nullptr);
  conn.setProperty(Props::weight, weight, nullptr);
  conn.setProperty(Props::phase, phase, nullptr);
  target.addChild(conn, -1, nullptr);
  return conn;
}

void removeConnection(juce::ValueTree target, int sourceId) {
  juce::ValueTree conn = target.getChildWithProperty(Props::sourceId, sourceId);
  if (conn.isValid())
    target.removeChild(conn, nullptr);
}

// for iteration (ValueTree has no built-in forEach)
void forEachNode(const juce::ValueTree& root,
                 const std::function<void(juce::ValueTree)>& fn) {
  for (int i = 0; i < root.getNumChildren(); ++i) {
    fn(root.getChild(i));
  }
}

void forEachConnection(const juce::ValueTree& node,
                       const std::function<void(juce::ValueTree)>& fn) {
  for (int i = 0; i < node.getNumChildren(); ++i) {
    juce::ValueTree child = node.getChild(i);
    if (child.hasType(IDs::CONNECTION))
      fn(child);
  }
}

}  // namespace neurythmic::NetworkState
