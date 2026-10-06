#include "../include/Neurythmic/NetworkViewComponent.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "../include/Neurythmic/GraphGeometry.h"
#include "NeurythmicPluginAssets.h"
#include "../include/Neurythmic/NetworkState.h"

namespace neurythmic {

namespace {

juce::Colour scaleRGB(juce::Colour c, float m) {
  return juce::Colour::fromFloatRGBA(c.getFloatRed() * m, c.getFloatGreen() * m,
                                     c.getFloatBlue() * m, 1.0f);
}

// Snap a frequency multiple to the legacy display increments (1/16 below 1,
// 1/8 above, with 1/3 special-cased) and format it as a trimmed decimal.
juce::String formatFreqMultiple(double m) {
  if (m < 0.0)
    m = 0.0;

  double snapped;
  if (std::fabs(m - 1.0 / 3.0) < 0.05) {
    snapped = 1.0 / 3.0;
  } else if (m < 1.0) {
    snapped = std::round(m * 16.0) / 16.0;
  } else {
    snapped = std::round(m * 8.0) / 8.0;
  }

  juce::String s(std::round(snapped * 1000.0) / 1000.0, 3);
  while (s.endsWith("0"))
    s = s.dropLastCharacters(1);
  if (s.endsWith("."))
    s = s.dropLastCharacters(1);
  return s;
}

}  // namespace

NetworkViewComponent::NetworkViewComponent(NetworkController& controller)
    : _controller(controller) {
  auto typeface = juce::Typeface::createSystemTypefaceFor(
      neurythmic::assets::InterMedium_ttf,
      neurythmic::assets::InterMedium_ttfSize);
  _font = juce::Font(juce::FontOptions(typeface).withHeight(
      ConfigManager::get().fontSizeNodeInfo));
}

void NetworkViewComponent::update() {
  repaint();
}

void NetworkViewComponent::resized() {
  repaint();
}

void NetworkViewComponent::paint(juce::Graphics& g) {
  const auto& cfg = ConfigManager::get();
  g.fillAll(juce::Colour(25, 25, 30));

  const float minDim = static_cast<float>(std::min(getWidth(), getHeight()));
  if (minDim <= 0.0f)
    return;
  const float scaling = std::min(1.0f, minDim / 1000.0f);

  drawConnections(g, cfg, scaling, minDim);
  drawNodes(g, cfg, scaling, minDim);
  drawLabels(g, cfg, scaling, minDim);
}

void NetworkViewComponent::drawConnections(juce::Graphics& g,
                                           const ConfigManager& cfg,
                                           float scaling,
                                           float minDim) {
  std::map<int, juce::Point<float>> pos;
  for (int id : _controller.getNodeIds())
    pos[id] = toPixel(_controller.getNodePosition(id), minDim);

  for (const auto& conn : _controller.getConnections()) {
    const juce::Point<float> from = pos[conn.sourceId];
    const juce::Point<float> to = pos[conn.targetId];
    const float weight = static_cast<float>(conn.weight);
    const bool dotted = weight < 0.0001f;
    const bool selected =
        _controller.isConnectionSelected(conn.sourceId, conn.targetId);

    const auto geo = GraphGeometry::makeConnectionGeometry(
        from, to, conn.isParentEdge, scaling, cfg);

    juce::Colour colour =
        selected
            ? cfg.selectedColour
            : juce::Colours::lightgrey.interpolatedWith(
                  cfg.connColour, GraphGeometry::getColourScale(weight, cfg));
    if (!selected && dotted)
      colour = scaleRGB(colour, cfg.inactiveBrightnessMult);
    const float thickness = GraphGeometry::getLineWidth(weight, cfg);

    juce::Path path;
    if (conn.isParentEdge) {
      path.startNewSubPath(geo.start);
      path.lineTo(geo.end);
    } else {
      const auto arcPts =
          GraphGeometry::makeArc(geo.arcCentre, geo.radius, geo.startAngle,
                                 geo.endAngle, cfg.pointsInArc, cfg);
      path.startNewSubPath(arcPts[1]);
      for (size_t i = 2; i < arcPts.size() - 1; ++i)
        path.lineTo(arcPts[i]);
    }

    g.setColour(colour);
    if (dotted) {
      juce::Path dashed;
      const float dashes[2] = {6.0f, 4.0f};
      juce::PathStrokeType(thickness, juce::PathStrokeType::mitered,
                           juce::PathStrokeType::butt)
          .createDashedStroke(dashed, path, dashes, 2);
      g.fillPath(dashed);
    } else {
      g.strokePath(
          path, juce::PathStrokeType(thickness, juce::PathStrokeType::mitered,
                                     juce::PathStrokeType::butt));
    }

    const auto tri = GraphGeometry::makeArrowHead(
        geo.arrowTip, geo.arrowAngleDeg, weight, cfg);
    juce::Path arrow;
    arrow.addTriangle(tri[0].getX(), tri[0].getY(), tri[1].getX(),
                      tri[1].getY(), tri[2].getX(), tri[2].getY());
    g.fillPath(arrow);
  }
}

void NetworkViewComponent::drawNodes(juce::Graphics& g,
                                     const ConfigManager& cfg,
                                     float scaling,
                                     float minDim) {
  for (int id : _controller.getNodeIds()) {
    const float intensity =
        static_cast<float>(_controller.getNodeIntensity(id));
    const float bright = GraphGeometry::getNodeBrightness(intensity, cfg);
    const bool selected = _controller.isNodeSelected(id);
    const juce::Colour colour =
        selected ? cfg.selectedColour : scaleRGB(cfg.getNodeColour(id), bright);
    const float thickness = GraphGeometry::getNodeSpread(intensity, cfg);
    const juce::Point<float> p =
        toPixel(_controller.getNodePosition(id), minDim);

    g.setColour(colour);
    if (id == NetworkState::kRootNodeId) {
      const float haloRadius = cfg.nodeRadius * cfg.node0HaloSize * scaling;
      g.drawEllipse(p.getX() - haloRadius, p.getY() - haloRadius,
                    haloRadius * 2.0f, haloRadius * 2.0f, thickness);
    }
    const float radius = cfg.nodeRadius * scaling;
    g.drawEllipse(p.getX() - radius, p.getY() - radius, radius * 2.0f,
                  radius * 2.0f, thickness);
  }
}

void NetworkViewComponent::drawLabels(juce::Graphics& g,
                                      const ConfigManager& cfg,
                                      float scaling,
                                      float minDim) {
  const double rootFreq =
      _controller.getNodeFrequency(NetworkState::kRootNodeId);

  g.setColour(cfg.fontNodeInfoColour);
  g.setFont(_font);
  const float labelHeight = _font.getHeight();

  for (int id : _controller.getNodeIds()) {
    auto np = _controller.getNodePosition(id);
    const float cx = np.getX() * minDim;
    const float cy = np.getY() * minDim;
    const float visualRadius =
        (id == NetworkState::kRootNodeId)
            ? cfg.nodeRadius * cfg.node0HaloSize * scaling
            : cfg.nodeRadius * scaling;

    const double freq = _controller.getNodeFrequency(id);
    const double mult = (rootFreq > 0.0) ? freq / rootFreq : 1.0;
    g.drawText(formatFreqMultiple(mult),
               juce::Rectangle<float>(cx - 40.0f,
                                      cy - visualRadius - labelHeight - 6.0f,
                                      80.0f, labelHeight),
               juce::Justification::centred);

    const int barDivision = _controller.getNodeBarDivision(id);
    if (barDivision > 0) {
      g.drawText(juce::String(barDivision),
                 juce::Rectangle<float>(cx - 40.0f, cy + visualRadius + 6.0f,
                                        80.0f, labelHeight),
                 juce::Justification::centred);
    }
  }
}

// interaction -----------------------------------------------------------
juce::Point<float> NetworkViewComponent::toPixel(juce::Point<float> normalised,
                                                 float minDim) const {
  return juce::Point<float>(normalised.getX() * minDim,
                            normalised.getY() * minDim);
}

juce::Point<float> NetworkViewComponent::toNormalised(juce::Point<float> pixel,
                                                      float minDim) const {
  if (minDim <= 0.0f)
    return {};
  return juce::Point<float>(pixel.getX() / minDim, pixel.getY() / minDim);
}

void NetworkViewComponent::focusNode(int nodeId, juce::Point<float> pixel) {
  NetworkController::Focus f;
  f.type = (nodeId == NetworkState::kRootNodeId)
               ? NetworkController::FocusType::RootNode
               : NetworkController::FocusType::ChildNode;
  f.nodeId = nodeId;
  f.cursorPos = pixel;
  _controller.setFocus(f);
}

void NetworkViewComponent::focusConnection(int from,
                                           int to,
                                           juce::Point<float> pixel) {
  NetworkController::Focus f;
  f.type = (_controller.getNodeParent(to) == from)
               ? NetworkController::FocusType::ParentChildEdge
               : NetworkController::FocusType::InputEdge;
  f.nodeId = to;
  f.connectionFromId = from;
  f.connectionToId = to;
  f.cursorPos = pixel;
  _controller.setFocus(f);
}

void NetworkViewComponent::mouseDown(const juce::MouseEvent& e) {
  const float minDim = static_cast<float>(std::min(getWidth(), getHeight()));
  const juce::Point<float> pixel = e.position;
  const juce::Point<float> norm = toNormalised(pixel, minDim);
  const int nodeId = _controller.isNodeAtPoint(norm);

  // Right-click: select + focus only; the context menu arrives in Phase 5.
  if (e.mods.isRightButtonDown()) {
    if (nodeId >= 0) {
      _controller.clearSelection();
      _controller.setNodeSelected(nodeId, true);
      focusNode(nodeId, pixel);
    } else {
      const auto conn = _controller.connectionAtPoint(pixel, minDim);
      if (conn.first >= 0) {
        _controller.clearSelection();
        _controller.selectConnection(conn.first, conn.second);
        focusConnection(conn.first, conn.second, pixel);
      } else {
        _controller.clearSelection();
        _controller.clearFocus();
      }
    }
    return;
  }

  if (nodeId >= 0) {
    // Set focus first so the previous node becomes the connection source.
    focusNode(nodeId, pixel);
    const auto prev = _controller.getPrevFocus();

    if (e.mods.isShiftDown()) {
      if (prev.nodeId >= 0 && prev.nodeId != nodeId)
        _controller.toggleConnection(prev.nodeId, nodeId);
      return;
    }
    if (e.mods.isAltDown()) {
      _controller.resetNode(nodeId);
      return;
    }
    if (e.mods.isCommandDown()) {
      _controller.toggleNodeSelected(nodeId);
    } else {
      _controller.clearSelection();
      _controller.setNodeSelected(nodeId, true);
    }
    _controller.setNodePositionOffsets(norm);
    return;
  }

  const auto conn = _controller.connectionAtPoint(pixel, minDim);
  if (conn.first >= 0) {
    _controller.clearSelection();
    _controller.selectConnection(conn.first, conn.second);
    focusConnection(conn.first, conn.second, pixel);
    return;
  }

  _controller.clearSelection();
  _controller.clearFocus();
}

void NetworkViewComponent::mouseDrag(const juce::MouseEvent& e) {
  const float minDim = static_cast<float>(std::min(getWidth(), getHeight()));
  const juce::Point<float> norm = toNormalised(e.position, minDim);
  const auto focus = _controller.getFocus();

  if (e.mods.isLeftButtonDown()) {
    if (focus.type == NetworkController::FocusType::RootNode ||
        focus.type == NetworkController::FocusType::ChildNode) {
      if (_controller.canIDragHere(norm, focus.nodeId))
        _controller.moveSelectedNodes(norm);
    }
    return;
  }

  // Right-drag on empty space pans the whole graph.
  if (focus.type == NetworkController::FocusType::None)
    _controller.moveAllNodes(norm);
}

void NetworkViewComponent::mouseUp(const juce::MouseEvent&) {
  _controller.endMoveAllNodes();
}

}  // namespace neurythmic
