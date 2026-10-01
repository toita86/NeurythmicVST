#include "../include/Neurythmic/NetworkViewComponent.h"

#include <algorithm>
#include <cmath>
#include <map>

#include "../include/Neurythmic/GraphGeometry.h"
#include "NeurythmicPluginAssets.h"
#include "../include/Neurythmic/NetworkState.h"

namespace neurythmic {

namespace {

constexpr float kPiFloat = 3.14159265358979323846f;

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
  auto toPixel = [&](juce::Point<float> n) {
    return juce::Point<float>(n.getX() * minDim, n.getY() * minDim);
  };

  std::map<int, juce::Point<float>> pos;
  for (int id : _controller.getNodeIds())
    pos[id] = toPixel(_controller.getNodePosition(id));

  for (const auto& conn : _controller.getConnections()) {
    const juce::Point<float> from = pos[conn.sourceId];
    const juce::Point<float> to = pos[conn.targetId];
    const float weight = static_cast<float>(conn.weight);
    const bool dotted = weight < 0.0001f;
    const float colourScale = GraphGeometry::getColourScale(weight, cfg);
    juce::Colour colour =
        juce::Colours::lightgrey.interpolatedWith(cfg.connColour, colourScale);
    if (dotted)
      colour = scaleRGB(colour, cfg.inactiveBrightnessMult);
    const float thickness = GraphGeometry::getLineWidth(weight, cfg);

    juce::Path path;
    juce::Point<float> arrowTip;
    float arrowAngleDeg = 0.0f;

    if (conn.isParentEdge) {
      const float distFromCentre = cfg.nodeRadius * cfg.node0HaloSize * scaling;
      const juce::Point<float> start =
          GraphGeometry::projectToCircumference(from, to, distFromCentre);
      const juce::Point<float> end = GraphGeometry::projectToCircumference(
          to, from, distFromCentre + cfg.arrowHeadSize);
      arrowTip =
          GraphGeometry::projectToCircumference(to, from, distFromCentre);

      path.startNewSubPath(start);
      path.lineTo(end);

      const juce::Point<float> dv = start - arrowTip;
      arrowAngleDeg = std::atan2(-dv.getX(), dv.getY()) * 180.0f / kPiFloat;
    } else {
      const auto ie = GraphGeometry::makeInputEdge(from, to, scaling, cfg);
      const auto arcPts =
          GraphGeometry::makeArc(ie.arcCentre, ie.radius, ie.startAngle,
                                 ie.endAngle, cfg.pointsInArc, cfg);
      path.startNewSubPath(arcPts[1]);
      for (size_t i = 2; i < arcPts.size() - 1; ++i)
        path.lineTo(arcPts[i]);
      arrowTip = ie.drawArrowEnd;
      arrowAngleDeg = ie.arrowAngleDeg;
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

    const auto tri =
        GraphGeometry::makeArrowHead(arrowTip, arrowAngleDeg, weight, cfg);
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
  auto toPixel = [&](juce::Point<float> n) {
    return juce::Point<float>(n.getX() * minDim, n.getY() * minDim);
  };

  for (int id : _controller.getNodeIds()) {
    const float intensity =
        static_cast<float>(_controller.getNodeIntensity(id));
    const float bright = GraphGeometry::getNodeBrightness(intensity, cfg);
    const juce::Colour colour = scaleRGB(cfg.getNodeColour(id), bright);
    const float thickness = GraphGeometry::getNodeSpread(intensity, cfg);
    const juce::Point<float> p = toPixel(_controller.getNodePosition(id));

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

}  // namespace neurythmic
