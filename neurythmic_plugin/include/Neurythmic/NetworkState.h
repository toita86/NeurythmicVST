#pragma once

#include "juce_core/juce_core.h"
#include "juce_data_structures/juce_data_structures.h"

/*
 *
 * Right now the network only exists in two disconnected places:
 1. The engine (CPG/MatsuNode) — the real oscillator network, with nodes keyed
 by unsigned ID.
 2. The old frontend (matuoka_frontend) — a flat GUI_node _nodes[16] array of
 structs, each with status (inactive/requested/active), changed, changedPosition
 dirty flags, and a pile of fields.

 The flat array is the thing we're replacing. Its problems:
 - State is duplicated — the GUI array mirrors the engine,
    and they drift out of sync (that's why the old code had updateNetwork()
    manually reconciling the two).
 - Dirty flags are coarse — changed/changedPosition
    mean "something changed, redraw everything". No way to know what changed.
 - UI coupling — components have to reach into
    getNodeRW(id) and mutate structs directly, then remember to set changed =
 true.

 This implementation introduces the single source of truth: a juce::ValueTree
 that is the network model. The engine and the UI both read/write
 through it (via NetworkController t.b.i), and anything that cares
 about a property can be notified automatically.
 */

namespace neurythmic::NetworkState {
/*
 * Identifier is a string pool. The first time you write juce::Identifier("id"),
 * JUCE interns the string; every later use of the same text returns the same
 * pooled pointer. So:
 * - Property lookups in a ValueTree are hashmap lookups on interned keys —
 * effectively O(1).
 * - Comparing two identifiers is a pointer comparison, not a strcmp.
 * That's why we define every property name once as a constant, and never write
 * the raw string literal "freq" in code again. (Mismatched strings would
 * silently become two different pooled identifiers.)
 */
namespace IDs {
inline const juce::Identifier NETWORK{"Network"};
inline const juce::Identifier NODE{"Node"};
inline const juce::Identifier CONNECTION{"Connection"};
}  // namespace IDs

namespace Props {
// Node properties
inline const juce::Identifier id{"id"};
inline const juce::Identifier parentId{"parentId"};
inline const juce::Identifier freq{"freq"};
inline const juce::Identifier phaseOffset{"phaseOffset"};
inline const juce::Identifier noise{"noise"};
inline const juce::Identifier synchMode{"synchMode"};
inline const juce::Identifier positionX{"positionX"};
inline const juce::Identifier positionY{"positionY"};
inline const juce::Identifier quantGrid{"quantGrid"};
inline const juce::Identifier quantMultiple{"quantMultiple"};
inline const juce::Identifier quantOffset{"quantOffset"};
inline const juce::Identifier quantAmount{"quantAmount"};
// Connection properties
inline const juce::Identifier sourceId{"sourceId"};
inline const juce::Identifier weight{"weight"};
inline const juce::Identifier phase{"phase"};
inline const juce::Identifier scaleFactor{"scaleFactor"};
}  // namespace Props

constexpr int kRootNodeId = 0;
constexpr int kNoParentId = -1;
constexpr int kMaxNodes = 16;

// Network tree containgin a single root node with id = 0
juce::ValueTree initEmptyNetwork();

// Node child with property id == nodeId; invalid ValueTree if absent.
juce::ValueTree getNode(const juce::ValueTree& root, int nodeId);

// Adds a Node child. When parentId != kNoParentId, also adds a parent-child
// Connection child (sourceId == parentId). Returns the new node handle.
juce::ValueTree createNode(juce::ValueTree root,
                           int nodeId,
                           int parentId,
                           float posX,
                           float posY);

// Removes the Node child with the given id (and its Connection children).
void removeNode(juce::ValueTree root, int nodeId);

// Adds a Connection child to `target`, unless one with that sourceId exists.
// `scaleFactor` is the persistent multiplier from which `weight` is derived via
// `calcWeight(distance, scaleFactor)`; it survives node drags.
juce::ValueTree createConnection(juce::ValueTree target,
                                 int sourceId,
                                 double weight,
                                 double phase,
                                 double scaleFactor = 1.0);

// Removes the Connection child on `target` whose sourceId matches.
void removeConnection(juce::ValueTree target, int sourceId);

// Calls fn once per Node child.
void forEachNode(const juce::ValueTree& root,
                 const std::function<void(juce::ValueTree)>& fn);

// Calls fn once per Connection child of `node`.
void forEachConnection(const juce::ValueTree& node,
                       const std::function<void(juce::ValueTree)>& fn);

}  // namespace neurythmic::NetworkState
