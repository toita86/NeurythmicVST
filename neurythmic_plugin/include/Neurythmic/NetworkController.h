#pragma once

#include <map>
#include <set>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "ConfigManager.h"
#include "FlashEnvelope.h"
#include "MatsuokaEngine.h"
#include "NetworkState.h"

/*
 * The controller's role (recap)
engine (audio thread, queued actions)
      ^
      │
      NetworkController
      bridges them ──────────── > ValueTree (message thread, source of truth)

Rule for every mutator: ValueTree first, engine second, then doQueuedActions().
*/

namespace neurythmic {

class NetworkController {
public:
  enum class FocusType : char {
    RootNode,
    ChildNode,
    ParentChildEdge,
    InputEdge,
    Menu,
    None
  };
  enum class ActionState : char {
    AddChild,
    AddInput,
    SetConnection,
    DeleteNode,
    None
  };

  struct Focus {
    FocusType type = FocusType::None;
    int nodeId = -1;
    int connectionFromId = -1;
    int connectionToId = -1;
    juce::Point<float> cursorPos;
    ActionState action = ActionState::None;
  };

  NetworkController(MatsuokaEngine& engine,
                    ConfigManager& config,
                    double frameRate = 30.0);

  juce::ValueTree& getTree() { return _network; }
  const juce::ValueTree& getTree() const { return _network; }

  // Node lifecycle
  int createChild(int parentId);
  void deleteNode(int nodeId);
  void clear();

  // Connection lifecycle
  void addConnection(int from, int to);
  void removeConnection(int from, int to);
  void setConnectionScaleFactor(int from, int to, double scale);
  void updateConnectionPhase(int from, int to, double phase);
  double calcWeight(int from, int to, double scale) const;
  bool getIsConnected(int from, int to) const;
  // Shift+click: remove if already connected (and not the parent edge),
  // otherwise add with the default input-edge scale factor.
  void toggleConnection(int from, int to);

  // Node parameter control (Phase 5 menus; ValueTree first, engine second)
  void setNodeFrequency(int nodeId, double freq, bool inherit);
  void setNodeSelfNoise(int nodeId, double amount);
  double getNodeSelfNoise(int nodeId) const;
  void setNodePhaseOffset(int nodeId, double phase);
  double getNodePhaseOffset(int nodeId) const;
  void setNodeSynchMode(int nodeId, MatsuNode::synchMode mode);
  MatsuNode::synchMode getNodeSynchMode(int nodeId) const;
  void setNodeQuantiseGrid(int nodeId, MatsuokaEngine::gridType grid);
  MatsuokaEngine::gridType getNodeQuantiseGrid(int nodeId) const;
  void setNodeQuantiseMultiple(int nodeId, float mult);
  float getNodeQuantiseMultiple(int nodeId) const;
  void setNodeQuantiseOffset(int nodeId, float off);
  float getNodeQuantiseOffset(int nodeId) const;
  void setNodeQuantiseAmount(int nodeId, float amount);
  float getNodeQuantiseAmount(int nodeId) const;

  // Connection menu getters
  double getConnectionWeight(int from, int to) const;
  double getConnectionPhase(int from, int to) const;
  double getConnectionScaleFactor(int from, int to) const;

  // Position + hit testing
  void moveNode(int nodeId, juce::Point<float> pos);
  int isNodeAtPoint(juce::Point<float> pos) const;
  bool canIDragHere(juce::Point<float> pos, int nodeId) const;
  // Connection hit test in pixel space (matches the rendered geometry). Returns
  // the (sourceId, targetId) pair, or {-1, -1} when nothing is hit.
  std::pair<int, int> connectionAtPoint(juce::Point<float> pixelPos,
                                        float minDim) const;

  // Selection (UI-transient; never serialized)
  bool isNodeSelected(int nodeId) const;
  void setNodeSelected(int nodeId, bool selected);
  bool toggleNodeSelected(int nodeId);
  void clearNodeSelection();
  void selectConnection(int from, int to);
  bool isConnectionSelected(int from, int to) const;
  void clearSelection();

  // Drag (normalised coordinates)
  void setNodePositionOffsets(juce::Point<float> pos);
  void moveSelectedNodes(juce::Point<float> pos);
  void moveAllNodes(juce::Point<float> pos);
  void endMoveAllNodes();

  // Reset a node to its engine defaults (Alt+click).
  void resetNode(int nodeId);

  // Focus
  void setFocus(Focus newFocus);
  const Focus& getFocus() const { return _focus; };
  const Focus& getPrevFocus() const { return _prevFocus; };
  void clearFocus();

  // Per-frame sync
  void updateFromEngine();
  double getNodeIntensity(int nodeId) const;  // flash envelope 0..1

  // Queries
  std::vector<int> getNodeIds() const;
  int getNodeCount() const;
  juce::Point<float> getNodePosition(int nodeId) const;
  double getNodeFrequency(int nodeId) const;  // reads the tree freq property
  int getNodeBarDivision(int nodeId) const;   // engine quantiser bar division
  int getNodeParent(int nodeId) const;  // reads the tree parentId property

  // Connection enumeration for the renderer (source of truth = ValueTree).
  struct ConnectionInfo {
    int sourceId = -1;
    int targetId = -1;
    double weight = 0.0;
    bool isParentEdge = false;
  };
  std::vector<ConnectionInfo> getConnections() const;

  // Replaces the whole network from a ValueTree (preset load):
  // clears the engine, re-adds nodes parent-before-child,
  // restores connections and parameters.
  void rebuild(const juce::ValueTree& newRoot);

private:
  MatsuokaEngine& _engine;
  ConfigManager& _config;
  juce::ValueTree _network;
  std::vector<FlashEnvelope> _flashEnvelopes;
  Focus _focus;
  Focus _prevFocus;
  double _frameRate;

  std::set<int> _selectedNodes;
  int _selectedConnectionFrom = -1;
  int _selectedConnectionTo = -1;
  std::vector<juce::Point<float>> _clickOffsets;
  bool _draggingAll = false;

  int nextFreeNodeId() const;
  juce::Point<float> nodePosition(int nodeId) const;
  void setNodePosition(int nodeId, juce::Point<float> pos);
  void recomputeConnectionWeight(int from, int to);
  void setNodeParameterDefaults(juce::ValueTree node);

  juce::Point<float> positionNewNode(int parentId);
  juce::Point<float> positionFirstNewNode(int parentId);
  juce::Point<float> positionOtherNewNode(int parentId);
  bool isLocationFree(juce::Point<float> pos) const;
};
}  // namespace neurythmic
