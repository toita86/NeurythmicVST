#include "../include/Neurythmic/NetworkController.h"

#include <cmath>
#include <stdexcept>

namespace neurythmic {

namespace {
constexpr double kPi = 3.14159265358979323846;

juce::Point<float> rotate(juce::Point<float> pt,
                          juce::Point<float> pivot,
                          double angleRad) {
  double s = std::sin(angleRad);
  double c = std::cos(angleRad);
  double dx = pt.getX() - pivot.getX();
  double dy = pt.getY() - pivot.getY();
  return {static_cast<float>(pivot.getX() + dx * c - dy * s),
          static_cast<float>(pivot.getY() + dx * s + dy * c)};
}
}  // namespace

// skeleton & ownership ------------------------------------------------
NetworkController::NetworkController(MatsuokaEngine& engine,
                                     ConfigManager& config,
                                     double frameRate)
    : _engine(engine),
      _config(config),
      _network(NetworkState::initEmptyNetwork()),
      _frameRate(frameRate) {
  _flashEnvelopes.reserve(NetworkState::kMaxNodes);
  for (int i = 0; i < NetworkState::kMaxNodes; ++i)
    _flashEnvelopes.emplace_back(frameRate);

  // Mirror the engine's root frequency into the tree.
  juce::ValueTree root =
      NetworkState::getNode(_network, NetworkState::kRootNodeId);
  root.setProperty(NetworkState::Props::freq, _engine.getNodeFrequency(0),
                   nullptr);
}

std::vector<int> NetworkController::getNodeIds() const {
  std::vector<int> ids;
  NetworkState::forEachNode(_network, [&](juce::ValueTree n) {
    ids.push_back(static_cast<int>(n.getProperty(NetworkState::Props::id)));
  });
  return ids;
}

int NetworkController::getNodeCount() const {
  return _network.getNumChildren();
}

juce::Point<float> NetworkController::getNodePosition(int nodeId) const {
  return nodePosition(nodeId);
}

// node lifecycle CRUD operations
// -------------------------------------------------------
int NetworkController::createChild(int parentId) {
  if (!NetworkState::getNode(_network, parentId).isValid())
    throw std::runtime_error("createChild: parent does not exist");

  int newId = nextFreeNodeId();
  if (newId < 0)
    return -1;  // node limit (16) reached

  juce::Point<float> pos = positionNewNode(parentId);
  double freq = _engine.getNodeFrequency(0) * _config.newNodeFreqMultiple;

  // ValueTree first
  juce::ValueTree node = NetworkState::createNode(_network, newId, parentId,
                                                  pos.getX(), pos.getY());
  node.setProperty(NetworkState::Props::freq, freq, nullptr);

  double weight =
      calcWeight(parentId, newId, _config.newParentChildConnWeightScale);
  juce::ValueTree pc =
      node.getChildWithProperty(NetworkState::Props::sourceId, parentId);
  if (pc.isValid())
    pc.setProperty(NetworkState::Props::weight, weight, nullptr);

  // Engine second
  _engine.addChild(parentId, newId);
  _engine.setNodeFrequency(newId, freq, false);
  _engine.setConnection(parentId, newId, weight);
  _engine.doQueuedActions();

  return newId;
}

void NetworkController::deleteNode(int nodeId) {
  if (nodeId == NetworkState::kRootNodeId)
    throw std::runtime_error("cannot delete root node");
  juce::ValueTree node = NetworkState::getNode(_network, nodeId);
  if (!node.isValid())
    return;

  bool hasChildren = false;
  NetworkState::forEachNode(_network, [&](juce::ValueTree n) {
    if (static_cast<int>(n.getProperty(NetworkState::Props::parentId)) ==
        nodeId)
      hasChildren = true;
  });
  if (hasChildren)
    throw std::runtime_error("cannot delete interior node");

  // Drop connections whose source is this node (they live on other targets).
  NetworkState::forEachNode(_network, [&](juce::ValueTree other) {
    if (static_cast<int>(other.getProperty(NetworkState::Props::id)) != nodeId)
      NetworkState::removeConnection(other, nodeId);
  });

  NetworkState::removeNode(_network, nodeId);  // ValueTree first
  _engine.deleteNode(nodeId);                  // Engine second
  _engine.doQueuedActions();
}

void NetworkController::clear() {
  _network = NetworkState::initEmptyNetwork();
  juce::ValueTree root =
      NetworkState::getNode(_network, NetworkState::kRootNodeId);
  root.setProperty(NetworkState::Props::freq, _engine.getNodeFrequency(0),
                   nullptr);
  _engine.clear();
  _engine.doQueuedActions();
}

// connection lifecycle -------------------------------------------------
double NetworkController::calcWeight(int from, int to, double scale) const {
  juce::Point<float> a = nodePosition(from);
  juce::Point<float> b = nodePosition(to);
  double dist = std::hypot(b.getX() - a.getX(), b.getY() - a.getY());
  dist -= _config.nodeDistWeightScalingStart;
  if (dist < 0.0)
    dist = 0.0;
  double limit = _config.nodeDistWeightScalingLimit;
  double scaled = dist > limit ? 1.0 : dist / limit;
  return scale * std::pow(1.0 - scaled, _config.nodeDistWeightScalingExp);
}

void NetworkController::addConnection(int from, int to) {
  juce::ValueTree target = NetworkState::getNode(_network, to);
  if (!target.isValid() || !NetworkState::getNode(_network, from).isValid())
    throw std::runtime_error("addConnection: node does not exist");

  double weight = calcWeight(from, to, _config.newConnWeightScale);
  NetworkState::createConnection(target, from, weight, 0.0);  // ValueTree first
  _engine.setConnection(from, to, weight);                    // Engine second
  _engine.doQueuedActions();
}

void NetworkController::removeConnection(int from, int to) {
  juce::ValueTree target = NetworkState::getNode(_network, to);
  if (!target.isValid())
    return;

  if (static_cast<int>(target.getProperty(NetworkState::Props::parentId)) ==
      from)
    throw std::runtime_error("cannot remove connection to parent");

  NetworkState::removeConnection(target, from);
  _engine.removeConnection(from, to);
  _engine.doQueuedActions();
}

void NetworkController::updateConnectionWeight(int from,
                                               int to,
                                               double weight) {
  juce::ValueTree target = NetworkState::getNode(_network, to);
  if (!target.isValid())
    return;
  juce::ValueTree conn =
      target.getChildWithProperty(NetworkState::Props::sourceId, from);
  if (conn.isValid())
    conn.setProperty(NetworkState::Props::weight, weight, nullptr);
  _engine.setConnection(from, to, weight);
  _engine.doQueuedActions();
}

void NetworkController::updateConnectionPhase(int from, int to, double phase) {
  juce::ValueTree target = NetworkState::getNode(_network, to);
  if (!target.isValid())
    return;
  juce::ValueTree conn =
      target.getChildWithProperty(NetworkState::Props::sourceId, from);
  if (conn.isValid())
    conn.setProperty(NetworkState::Props::phase, phase, nullptr);
  _engine.setConnectionPhaseOffset(from, to, phase);
  _engine.doQueuedActions();
}

// position + hit testing + focus ---------------------------------------
void NetworkController::moveNode(int nodeId, juce::Point<float> pos) {
  setNodePosition(nodeId, pos);
}

int NetworkController::isNodeAtPoint(juce::Point<float> pos) const {
  float r = _config.nodeClickableRadius;
  int hit = -1;
  NetworkState::forEachNode(_network, [&](juce::ValueTree n) {
    juce::Point<float> p =
        nodePosition(static_cast<int>(n.getProperty(NetworkState::Props::id)));
    if (p.getX() - r < pos.getX() && p.getX() + r > pos.getX() &&
        p.getY() - r < pos.getY() && p.getY() + r > pos.getY())
      hit = static_cast<int>(n.getProperty(NetworkState::Props::id));
  });
  return hit;
}

bool NetworkController::canIDragHere(juce::Point<float> pos, int nodeId) const {
  float d = _config.nodeCollideDistance;
  bool free = true;
  NetworkState::forEachNode(_network, [&](juce::ValueTree n) {
    int id = static_cast<int>(n.getProperty(NetworkState::Props::id));
    if (id == nodeId)
      return;
    juce::Point<float> p = nodePosition(id);
    if (p.getX() - d < pos.getX() && p.getX() + d > pos.getX() &&
        p.getY() - d < pos.getY() && p.getY() + d > pos.getY())
      free = false;
  });
  return free;
}

void NetworkController::setFocus(Focus newFocus) {
  if (_focus.type != FocusType::Menu)
    _prevFocus = _focus;
  _focus = newFocus;
}

void NetworkController::clearFocus() {
  _focus = Focus{};
}

// per-frame engine sync ------------------------------------------------
void NetworkController::updateFromEngine() {
  auto events = _engine.getEvents();  // consumes + clears (mutex-protected)
  for (const auto& e : events) {
    if (e.nodeID < static_cast<unsigned>(NetworkState::kMaxNodes))
      _flashEnvelopes[e.nodeID].trigger(static_cast<double>(e.velocity));
  }

  for (auto& env : _flashEnvelopes)
    env.step();

  // Reflect engine frequencies into the tree only when they change.
  for (unsigned nodeId : _engine.getNodeList()) {
    juce::ValueTree n =
        NetworkState::getNode(_network, static_cast<int>(nodeId));
    if (!n.isValid())
      continue;
    double freq = _engine.getNodeFrequency(nodeId);
    double old = static_cast<double>(n.getProperty(NetworkState::Props::freq));
    if (freq != old)
      n.setProperty(NetworkState::Props::freq, freq, nullptr);
  }
}

double NetworkController::getNodeIntensity(int nodeId) const {
  if (nodeId < 0 || nodeId >= static_cast<int>(_flashEnvelopes.size()))
    return 0.0;
  return _flashEnvelopes[nodeId].getValue();
}

// private helpers -------------------------------------------------------------
int NetworkController::nextFreeNodeId() const {
  for (int id = 1; id < NetworkState::kMaxNodes; ++id)
    if (!NetworkState::getNode(_network, id).isValid())
      return id;
  return -1;
}

juce::Point<float> NetworkController::nodePosition(int nodeId) const {
  juce::ValueTree n = NetworkState::getNode(_network, nodeId);
  if (!n.isValid())
    return {};
  return {static_cast<float>(n.getProperty(NetworkState::Props::positionX)),
          static_cast<float>(n.getProperty(NetworkState::Props::positionY))};
}

void NetworkController::setNodePosition(int nodeId, juce::Point<float> pos) {
  juce::ValueTree n = NetworkState::getNode(_network, nodeId);
  if (!n.isValid())
    return;
  n.setProperty(NetworkState::Props::positionX, pos.getX(), nullptr);
  n.setProperty(NetworkState::Props::positionY, pos.getY(), nullptr);
}

juce::Point<float> NetworkController::positionNewNode(int parentId) {
  return getNodeCount() == 1 ? positionFirstNewNode(parentId)
                             : positionOtherNewNode(parentId);
}

juce::Point<float> NetworkController::positionFirstNewNode(int parentId) {
  juce::Point<float> parent = nodePosition(parentId);
  juce::Point<float> attempt =
      parent + juce::Point<float>(0.0f, _config.nodeSpawnDistance);
  for (int i = 0; i < 4; ++i) {
    if (isLocationFree(attempt))
      return attempt;
    attempt = rotate(attempt, parent, kPi / 2.0);
  }
  throw std::runtime_error("could not find free position for node");
}

juce::Point<float> NetworkController::positionOtherNewNode(int parentId) {
  juce::Point<float> parent = nodePosition(parentId);
  juce::Point<float> origin = nodePosition(NetworkState::kRootNodeId);
  double parentAngle =
      std::atan2(parent.getY() - origin.getY(), parent.getX() - origin.getX());
  double distance = _config.nodeSpawnDistance;

  juce::Point<float> attempt =
      parent + juce::Point<float>(0.0f, static_cast<float>(distance));
  attempt = rotate(attempt, parent, parentAngle);
  if (isLocationFree(attempt))
    return attempt;

  double rotateAngle = _config.nodeSpawnDistance / distance;
  double rotateOffset = 0.0;
  while (distance < 1.0) {
    unsigned rotateTries = static_cast<unsigned>(kPi / rotateAngle);
    juce::Point<float> fwd = attempt;
    juce::Point<float> back = rotate(fwd, parent, -rotateAngle);
    while (rotateTries-- > 0) {
      fwd = rotate(fwd, parent, rotateAngle);
      if (isLocationFree(fwd))
        return fwd;
      back = rotate(back, parent, -rotateAngle);
      if (isLocationFree(back))
        return back;
    }
    distance += _config.nodeSpawnDistance;
    rotateAngle = _config.nodeSpawnDistance / distance;
    rotateOffset = rotateAngle / 2.0;
    juce::Point<float> raw =
        parent + juce::Point<float>(0.0f, static_cast<float>(distance));
    attempt = rotate(raw, parent, rotateOffset + parentAngle);
    if (isLocationFree(attempt))
      return attempt;
  }
  throw std::runtime_error("could not find free position for node");
}

bool NetworkController::isLocationFree(juce::Point<float> pos) const {
  return isNodeAtPoint(pos) == -1;
}

}  // namespace neurythmic
