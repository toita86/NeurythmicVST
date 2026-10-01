#pragma once

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
  void updateConnectionWeight(int from, int to, double weight);
  void updateConnectionPhase(int from, int to, double phase);
  double calcWeight(int from, int to, double scale) const;

  // Position + hit testing
  void moveNode(int nodeId, juce::Point<float> pos);
  int isNodeAtPoint(juce::Point<float> pos) const;
  bool canIDragHere(juce::Point<float> pos, int nodeId) const;

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

  int nextFreeNodeId() const;
  juce::Point<float> nodePosition(int nodeId) const;
  void setNodePosition(int nodeId, juce::Point<float> pos);

  juce::Point<float> positionNewNode(int parentId);
  juce::Point<float> positionFirstNewNode(int parentId);
  juce::Point<float> positionOtherNewNode(int parentId);
  bool isLocationFree(juce::Point<float> pos) const;
};
}  // namespace neurythmic
